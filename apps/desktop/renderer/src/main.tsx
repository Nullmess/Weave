import './web-preview';
import React,{useCallback,useEffect,useRef,useState} from 'react';
import {createRoot} from 'react-dom/client';
import ReactMarkdown from 'react-markdown';
import remarkGfm from 'remark-gfm';
import rehypeRaw from 'rehype-raw';
import rehypeSanitize,{defaultSchema} from 'rehype-sanitize';
import {FlipperBridge} from './flipper-bridge';
import {NET_REQUEST,NET_RESPONSE,NET_READY,NET_LINK,parseNetworkRequest,networkReply,networkReady,networkHeartbeat,NET_EVENT,NET_EVENT_READY,networkEvent,networkEventReady} from './network-mailbox';
import {screenPixels,type FileItem} from './flipper-rpc';
import {createZip,type ArchiveItem} from './archive';
import './theme.css';

const SERIAL='8fe5b3d5-2e7f-4a98-2a48-7acc60fe0000';
// The Flipper advertises 0x3080..0x3083 (color variants), not its 128-bit RPC
// GATT UUID. A custom hostname is not required to start with Flipper/Weave.
const BLE_FILTERS=[0x3080,0x3081,0x3082,0x3083].map(service=>({services:[service]}));
const BLE_PAIR_FILTERS=[...BLE_FILTERS,{namePrefix:'Flipper'},{namePrefix:'Weave'}];
const TX='19ed82ae-ed21-4c9d-4145-228e61fe0000';
const RX='19ed82ae-ed21-4c9d-4145-228e62fe0000';
const FLOW='19ed82ae-ed21-4c9d-4145-228e63fe0000';
const BATTERY='battery_service';
const CATALOG_LIMIT=12000;
const sleep=(ms:number)=>new Promise<void>(r=>setTimeout(r,ms));
type Page='Home'|'Apps'|'Terminal'|'Files'|'Settings'|'Info';
type Transport='USB'|'Bluetooth'|null;
type CatalogEntry={id:string;archiveId:string;title:string;category:string;description:string;icon:string;version:string;page:string;sourceUrl?:string;manifestUrl?:string;screenshots?:string[];size?:number;views?:number;updated?:number;descriptionUrl?:string};
type InstalledEntry={name:string;path:string};
type NativeInstallStatus={id:string;title:string;phase:'waiting'|'downloading'|'installing'|'done'|'failed';percent:number;eta:number};
type UpdateKind='app'|'firmware';
type UpdateStatus='unknown'|'checking'|'current'|'available'|'downloading';
type UpdateView={status:UpdateStatus;latest:string;progress:number;error:string};
const emptyUpdate=():UpdateView=>({status:'unknown',latest:'',progress:0,error:''});
const compareVersions=(a:string,b:string):number=>{
  if(!/^\d+\.\d+\.\d+$/.test(a)||!/^\d+\.\d+\.\d+$/.test(b))return 0;
  const left=a.split('.').map(Number),right=b.split('.').map(Number);
  for(let i=0;i<3;i++)if(left[i]!==right[i])return left[i]>right[i]?1:-1;
  return 0;
};
const pathName=(path:string)=>path.substring(path.lastIndexOf('/')+1);
function uniqueName(name:string,reserved:Set<string>){
  let stem=name,ext='';const dot=name.lastIndexOf('.');if(dot>0){stem=name.slice(0,dot);ext=name.slice(dot);}
  if(!reserved.has(name)){reserved.add(name);return name;}
  let n=1;while(reserved.has(`${stem}_${n}${ext}`))n++;
  const candidate=`${stem}_${n}${ext}`;reserved.add(candidate);return candidate;
}
// Only explicit published FAP/build sizes, never the size of a generic archive.
function catalogFapSize(node:any,depth=0,buildContext=false):number{
  if(depth>6||node==null||typeof node!=='object')return 0;
  if(Array.isArray(node)){
    for(const value of node.slice(0,60)){const bytes=catalogFapSize(value,depth+1,buildContext);if(bytes)return bytes;}
    return 0;
  }
  const valid=(v:any)=>{const n=Number(v);return Number.isFinite(n)&&n>0&&n<=100_000_000?Math.round(n):0;};
  for(const key of ['fap_size','fapSize','fap_size_bytes','binary_size','build_size','fap_file_size']){
    const n=valid(node[key]);if(n)return n;
  }
  if(String(node.filename||node.path||node.file_name||'').toLowerCase().endsWith('.fap')||
      (buildContext&&(node.target||node.api||node.firmware_api))){const n=valid(node.size);if(n)return n;}
  for(const key of ['latest_build','build','builds','artifacts','binary','binaries','releases','versions','assets','fap','application','data','result']){
    const bytes=catalogFapSize(node[key],depth+1,true);if(bytes)return bytes;
  }
  return 0;
}
function parseCatalog(payload:unknown):CatalogEntry[]{
  const body=payload as any;
  const raw=Array.isArray(body)?body:[body?.applications,body?.items,body?.data,body?.results].find(Array.isArray);
  if(!Array.isArray(raw))throw Error('Unexpected catalog response');
  return raw.map((it:any)=>{
    const id=String(it?.alias||it?.slug||it?.appid||it?.id||'');
    const title=String(it?.name||it?.title||'');
    const archived=String(it?._id||it?.id||'');
    const icon=String(it?.icon_url||it?.iconUrl||it?.icon?.url||it?.icon?.src||it?.icon||'');
    const shots=Array.isArray(it?.screenshots)?it.screenshots.map((v:any)=>typeof v==='string'?v:String(v?.url||v?.src||'')).filter((v:string)=>v.startsWith('https://')).slice(0,12):[];
    return {id,archiveId:/^[a-f0-9]{24}$/i.test(archived)?archived:'',title,category:String(it?.category||'Other'),description:String(it?.description||it?.short_description||''),
      icon:catalogAsset(icon)||'',version:String(typeof it?.version==='object'?it?.version?.version||'':it?.version||it?.latest_version?.version||''),
      page:`https://catalog.flipperzero.one/application/${/^[a-f0-9]{24}$/i.test(archived)?archived:id}/page`,
      sourceUrl:String(it?.repository||'').startsWith('https://github.com/')?String(it.repository):undefined,
      manifestUrl:typeof it?.manifestUrl==='string'?it.manifestUrl:undefined,
      descriptionUrl:typeof it?.descriptionUrl==='string'?it.descriptionUrl:undefined,
      screenshots:shots,views:Number(it?.views||0),updated:Number(it?.updated_at||0),size:catalogFapSize(it)};
  }).filter((it:CatalogEntry)=>/^[A-Za-z0-9_-]{2,100}$/.test(it.id)&&!!it.title);
}
// Public application record, used only if a catalog page omitted the app.
// No guessed compatible FAP, version or published build URL.
const PUBLIC_TOTP:CatalogEntry={id:'totp',archiveId:'672735a56cb04a64f7ee03ab',
  title:'Authenticator',category:'Tools',description:'Time-based and HMAC one-time password authenticator',
  icon:'',version:'',page:'https://catalog.flipperzero.one/application/672735a56cb04a64f7ee03ab/page'};
// Identical discovery strategy to Android: official archive first, then the
// upstream manifest index. A manifest-only app has no invented FAP/archive ID.
async function fetchCompleteCatalog(onUpdate:(items:CatalogEntry[])=>void,alive:()=>boolean){
  const byId=new Map<string,CatalogEntry>();
  let archiveError:unknown=null;
  for(let offset=0;offset<CATALOG_LIMIT&&alive();offset+=100){
    let items:CatalogEntry[];
    try{items=parseCatalog(await window.weave.catalog('',offset));}
    catch(error){archiveError=error;break;}
    let additions=0;
    for(const item of items){if(!byId.has(item.id)){byId.set(item.id,item);additions++;}}
    if(alive())onUpdate([...byId.values()]);
    if(items.length===0||additions===0)break;
    if(items.length<100)break;
  }
  if(alive()){
    try{
      const manifests=await window.weave.catalogManifests();
      for(const item of parseCatalog(manifests)){
        const existing=byId.get(item.id);
        if(existing)byId.set(item.id,{...item,...existing,
          icon:existing.icon||item.icon,description:existing.description||item.description,
          sourceUrl:existing.sourceUrl||item.sourceUrl,manifestUrl:existing.manifestUrl||item.manifestUrl,
          screenshots:existing.screenshots?.length?existing.screenshots:item.screenshots});
        else byId.set(item.id,item);
      }
    }catch(error){if(!byId.size)throw archiveError||error;}
  }
  if(!byId.has(PUBLIC_TOTP.id))byId.set(PUBLIC_TOTP.id,PUBLIC_TOTP);
  const complete=[...byId.values()].sort((a,b)=>a.title.localeCompare(b.title,undefined,{sensitivity:'base'}));
  if(alive())onUpdate(complete);
  return complete;
}
const BUNDLED_FIRMWARE_VERSION='1.0.0';
function formatFirmwareVersion(raw:string):string{
  const version=raw.match(/(?:^|\b)v?(\d+\.\d+\.\d+)(?:$|\b)/i)?.[1];
  const installed=version||(raw.toLowerCase().startsWith('source archive')?BUNDLED_FIRMWARE_VERSION:'');
  if(!installed)return 'Version unavailable';
  const parts=(s:string)=>s.split('.').map(Number);
  const left=parts(installed),right=parts(BUNDLED_FIRMWARE_VERSION);
  const newer=right.some((v,i)=>left.slice(0,i).every((n,j)=>n===right[j])&&v>left[i]);
  return newer?`v${installed} → v${BUNDLED_FIRMWARE_VERSION}`:`v${installed}`;
}
function saveBytes(name:string,data:Uint8Array){return window.weave.saveFile(name,Array.from(data));}

function catalogAsset(href:string|undefined,base?:string):string|undefined{
  if(!href)return;
  try{
    const u=new URL(href,base||(href.startsWith('/')&&!href.startsWith('//')?'https://catalog.flipperzero.one':undefined));
    if(u.protocol!=='https:'||u.username||u.password)return;
    // Images are loaded from the official catalog and GitHub publication origins.
    if(['raw.githubusercontent.com','github.com','catalog.flipperzero.one','cdn.flipperzero.one','avatars.githubusercontent.com','flipper.net','flipperzero.one'].includes(u.hostname))return u.toString();
  }catch{}
  return;
}
// Support GitHub README HTML elements without allowing embedded scripts, event handlers,
// unsafe inline styles, forms, iframes, SVG or external resource execution.
// Alignment is presentation-only; CSS supplies the actual responsive layout.
export const githubHtmlSchema={
  ...defaultSchema,
  tagNames:[...(defaultSchema.tagNames||[]),'div','span','section'],
  attributes:{...defaultSchema.attributes,
    div:['align'],span:['align'],section:['align'],
    h1:['align'],h2:['align'],h3:['align'],h4:['align'],
    p:['align'],table:['align'],thead:['align'],tbody:['align'],
    tr:['align'],td:['align','colSpan','rowSpan'],th:['align','colSpan','rowSpan'],
    img:['src','alt','title','width','height','align'],
    a:['href','title']
  },
};
const catalogImageCandidates=(href:string|undefined):string[]=>{
  const primary=catalogAsset(href);
  if(!primary)return [];
  const candidates=[primary];
  try{
    const uri=new URL(primary);
    if(uri.hostname==='raw.githubusercontent.com'&&/\.png$/i.test(uri.pathname)){
      const dir=uri.pathname.slice(0,uri.pathname.lastIndexOf('/')+1);
      const name=uri.pathname.slice(uri.pathname.lastIndexOf('/')+1);
      for(const folder of ['assets/','images/','icons/','fap_icon_assets/','resources/']){
        const candidate=catalogAsset(uri.origin+dir+folder+name);
        if(candidate&&!candidates.includes(candidate))candidates.push(candidate);
      }
    }
  }catch{}
  return candidates;
};
const appIconCache=new Map<string,string>();
const normalizeAppName=(name:string)=>name.toLocaleLowerCase().replace(/\.fap$/i,'').replace(/[\s_-]+/g,'');
const iconOf=(name:string)=>'./icons/'+name+'.svg';
function AppIcon({entry,className=''}:{entry?:CatalogEntry;className?:string}){
  const element=useRef<HTMLImageElement|null>(null);
  const failed=useRef<Set<string>>(new Set());
  const candidates=useRef<string[]>(catalogImageCandidates(entry?.icon));
  const attemptedLookup=useRef(false);
  const [source,setSource]=useState(()=>appIconCache.get(entry?.id||'')||catalogAsset(entry?.icon)||'');
  useEffect(()=>{
    failed.current.clear();attemptedLookup.current=false;
    candidates.current=[...new Set([...catalogImageCandidates(entry?.icon),...catalogImageCandidates(appIconCache.get(entry?.id||''))])];
    setSource(appIconCache.get(entry?.id||'')||catalogAsset(entry?.icon)||'');
  },[entry?.id,entry?.icon]);
  useEffect(()=>{
    if(!entry?.id||source||attemptedLookup.current)return;
    let active=true;
    const fetchIcon=()=>{
      if(attemptedLookup.current)return;
      attemptedLookup.current=true;
      void window.weave.catalogVisual(entry.id).then((result:any)=>{
        candidates.current=[...new Set([...candidates.current,...catalogImageCandidates(typeof result?.icon_url==='string'?result.icon_url:'')])];
        const next=candidates.current.find(url=>!failed.current.has(url));
        if(active&&next){appIconCache.set(entry.id,next);setSource(next);}
      }).catch(()=>{});
    };
    if(typeof IntersectionObserver==='undefined'||!element.current){fetchIcon();return()=>{active=false;};}
    const observer=new IntersectionObserver(items=>{
      if(items.some(item=>item.isIntersecting)){observer.disconnect();fetchIcon();}
    },{rootMargin:'220px'});
    observer.observe(element.current);
    return()=>{active=false;observer.disconnect();};
  },[entry?.id,source]);
  const nextImage=()=>{
    if(!source)return;
    failed.current.add(source);
    appIconCache.delete(entry?.id||'');
    const next=candidates.current.find(url=>!failed.current.has(url));
    setSource(next||'');
  };
  return <img ref={element} className={className} src={source||iconOf('ic_files_cube')}
    alt={entry?.title?`${entry.title} icon`:''} loading="lazy" onLoad={()=>{if(entry?.id&&source)appIconCache.set(entry.id,source);}} onError={nextImage}/>;
}
function CatalogTile({entry,onOpen}:{entry:CatalogEntry;onOpen:(entry:CatalogEntry)=>void}){
  return <button className="app-tile" onClick={()=>onOpen(entry)}><AppIcon entry={entry}/><b>{entry.title}</b><small>{entry.category}</small></button>;
}


/* Store optional Shop versions in a single private database rather than
 * leaving one .txt file for every FAP in /ext/apps. Native Flipper Shop uses
 * the same format and takes care of version updates for its own installs. */
async function rememberFapVersion(client:any,id:string,version:string,path:string){
  if(!/^[A-Za-z0-9_-]{2,80}$/.test(id)||
     !/^[A-Za-z0-9_.+-]{1,32}$/.test(version)||
     !/^\/ext\/apps\/(?:[A-Za-z0-9_. -]+\/)*[A-Za-z0-9_. -]+\.fap$/.test(path)||
     path.includes('..'))return;
  const db='/ext/.weave/shop.versions';
  let existing='';
  try{existing=new TextDecoder().decode(await client.read(db));}catch{}
  const rows=existing.split(/\r?\n/).filter((line:string)=>{
    const fields=line.split('|');
    return fields.length===3&&fields[0]!==id&&/^[A-Za-z0-9_-]{2,80}$/.test(fields[0]);
  });
  rows.push(`${id}|${version}|${path}`);
  if(rows.join('\n').length>64000)return;
  try{await client.mkdir('/ext/.weave');}catch{}
  await client.write(db,new TextEncoder().encode(rows.join('\n')+'\n'));
}

function App(){
  const [page,setPage]=useState<Page>('Home');
  const [transport,setTransport]=useState<Transport>(null);
  const [connected,setConnected]=useState(false);
  const [connecting,setConnecting]=useState(false);
  const [devices,setDevices]=useState<{id:string;label:string}[]>([]);
  const [selectedUsb,setSelectedUsb]=useState('');
  const [battery,setBattery]=useState<number|null>(null);
  const [firmware,setFirmware]=useState('—');
  const [appUpdate,setAppUpdate]=useState<UpdateView>(emptyUpdate);
  const [firmwareUpdate,setFirmwareUpdate]=useState<UpdateView>(emptyUpdate);
  const [flipperLogo,setFlipperLogo]=useState('');
  const [authorAvatar,setAuthorAvatar]=useState('');
  const [firmwareApi,setFirmwareApi]=useState('');
  const [deviceName,setDeviceName]=useState('Flipper Zero');
  const [logs,setLogs]=useState<string[]>(['Weave is ready. Connect a Flipper Zero via USB or Bluetooth.']);
  const [screenStatus,setScreenStatus]=useState('Flipper disconnected');
  const [filesRoot,setFilesRoot]=useState<'/int'|'/ext'>('/ext');
  const [filePath,setFilePath]=useState('/ext');
  const [entries,setEntries]=useState<FileItem[]>([]);
  const [selectedFiles,setSelectedFiles]=useState<string[]>([]);
  const [clipboard,setClipboard]=useState<{move:boolean;items:{path:string;directory:boolean}[]}|null>(null);
  const [fileBusy,setFileBusy]=useState(false);
  const [progress,setProgress]=useState(0);
  const [filesMessage,setFilesMessage]=useState('Connect your Flipper to browse files.');
  const [appMode,setAppMode]=useState<'Shop'|'Installed'|'Maj'>('Shop');
  const [search,setSearch]=useState('');
  const [catalog,setCatalog]=useState<CatalogEntry[]>([]);
  const [catalogError,setCatalogError]=useState('');
  const [catalogBusy,setCatalogBusy]=useState(false);
  const [catalogRefresh,setCatalogRefresh]=useState(0);
  const [detailBusy,setDetailBusy]=useState(false);
  const [galleryIndex,setGalleryIndex]=useState<number|null>(null);
  const [appDetails,setAppDetails]=useState<CatalogEntry|null>(null);
  const [installed,setInstalled]=useState<InstalledEntry[]>([]);
  const [appsBusy,setAppsBusy]=useState(false);
  const [appTransferBusy,setAppTransferBusy]=useState(false);
  const [appTransferProgress,setAppTransferProgress]=useState(0);
  const [service,setService]=useState(false);
  const [autostart,setAutostart]=useState(false);
  const [findingDevice,setFindingDevice]=useState(false);
  const findingAudio=useRef<AudioContext|null>(null);
  const findingInterval=useRef<ReturnType<typeof setInterval>|null>(null);
  const [terminalMode,setTerminalMode]=useState<'CLI'|'LOGS'>('CLI');
  const [terminalCommand,setTerminalCommand]=useState('');
  const [terminalOutput,setTerminalOutput]=useState('');
  const [cliReady,setCliReady]=useState(false);
  const [terminalBusy,setTerminalBusy]=useState(false);
  const bridge=useRef<FlipperBridge|null>(null);
  const canvas=useRef<HTMLCanvasElement|null>(null);
  const latestFrame=useRef<Uint8Array|null>(null);
  const usbRef=useRef(false);
  const connectionRef=useRef<Transport>(null);
  const bleRef=useRef<any>(null);
  const rememberedBle=useRef<any>(null);
  const usbCandidate=useRef<FlipperBridge|null>(null);
  const linkBusy=useRef(false);
  const lastUsbFailure=useRef<{id:string;at:number}|null>(null);
  const lastBleFailure=useRef(0);
  const pendingUsbClose=useRef(false);
  const detaching=useRef(false);
  const cliPrevious=useRef(new Uint8Array());
  const cliPolling=useRef(false);
  const keyChain=useRef(Promise.resolve());
  const frameReady=useRef(false);
  const nativeShopRequest=useRef('');
  const [nativeShopStatus,setNativeShopStatus]=useState<NativeInstallStatus|null>(null);
  const nativeDownloadHandle=useRef<{archiveId:string;update:(received:number,total:number,percent:number)=>void}|null>(null);
  const lastFindRequest=useRef('');
  const lastFirmwareCheckRequest=useRef('');
  const findBusy=useRef(false);
  const nativeShopBusy=useRef(false);
  const notify=useCallback((line:string)=>setLogs(prev=>[`${new Date().toLocaleTimeString()}  ${line}`,...prev].slice(0,300)),[]);
  const withBridge=()=>{const rpc=bridge.current;if(!rpc)throw Error('Connect your Flipper first');return rpc;};
  const report=(error:unknown)=>{const message=error instanceof Error?error.message:String(error);notify(message);setFilesMessage(message);};
  const draw=useCallback((pixels:Uint8Array)=>{
    latestFrame.current=pixels;frameReady.current=true;
    const target=canvas.current;if(!target)return;
    const ctx=target.getContext('2d');if(!ctx)return;
    const data=new ImageData(screenPixels(pixels),128,64);ctx.putImageData(data,0,0);
  },[]);
  const detach=useCallback(async()=>{
    if(detaching.current)return;detaching.current=true;
    const old=bridge.current;bridge.current=null;old?.close();
    try{if(connectionRef.current==='USB')await window.weave.disconnectUsb();else bleRef.current?.gatt?.disconnect();}catch{}
    bleRef.current=null;usbCandidate.current=null;connectionRef.current=null;usbRef.current=false;
    setTransport(null);setConnected(false);setCliReady(false);setBattery(null);
    frameReady.current=false;nativeShopRequest.current='';nativeShopBusy.current=false;nativeDownloadHandle.current=null;setNativeShopStatus(null);lastFindRequest.current='';lastFirmwareCheckRequest.current='';
    setScreenStatus('Flipper disconnected');
    setEntries([]);setInstalled([]);setFirmware('—');setFirmwareApi('');setDeviceName('Flipper Zero');
    detaching.current=false;
  },[]);
  useEffect(()=>{
    const unsubscribe=window.weave.onUsb(data=>{const target=usbCandidate.current||(usbRef.current?bridge.current:null);target?.receive(new Uint8Array(data));});
    const onStatus=window.weave.onStatus(s=>{
      if(/USB disconnected/i.test(s)){
        if(pendingUsbClose.current)return;
        if(usbCandidate.current)usbCandidate.current.close();
        if(connectionRef.current==='USB')void detach();
      }else notify(s);
    });
    return()=>{unsubscribe();onStatus();};
  },[notify,detach]);
  useEffect(()=>{if(page==='Home'&&latestFrame.current)draw(latestFrame.current);},[page,draw]);
  function createBridge(send:(packet:Uint8Array)=>Promise<void>,chunkSize:number,activate=true){
    const client=new FlipperBridge(send,chunkSize);client.onScreen=(pixels)=>{draw(pixels);setScreenStatus('');};client.onWarning=notify;
    if(activate)bridge.current=client;return client;
  }
  async function initialize(client:FlipperBridge,knownInfo?:Record<string,string>){
    const info=knownInfo||await client.deviceInfo();
    const name=info.hardware_name||info.hardware_model||info.firmware_name||'Flipper Zero';
    setDeviceName(name);setFirmware(formatFirmwareVersion(info.firmware_version||info.firmware_name||''));
    if(/^\d+\.\d+$/.test(`${info.firmware_api_major||''}.${info.firmware_api_minor||''}`))
      setFirmwareApi(`${info.firmware_api_major}.${info.firmware_api_minor}`);
    setConnected(true);setScreenStatus('Connecting to Flipper screen…');
    await client.startScreen();notify('Flipper protobuf RPC ready');
    // The native Flipper Shop reads this bounded catalog directly from SD.
    setTimeout(()=>{if(bridge.current===client)void syncNativeCatalog(client);},3000);
  }
  const installedUpdateVersion=(kind:UpdateKind):string=>{
    if(kind==='app')return '1.0.0';
    if(!connected)return '';
    return firmware.match(/v?(\d+\.\d+\.\d+)/i)?.[1]||'';
  };
  const checkUpdate=useCallback(async(kind:UpdateKind)=>{
    const set=kind==='app'?setAppUpdate:setFirmwareUpdate;
    const installed=installedUpdateVersion(kind);
    if(!installed){set(emptyUpdate());return;}
    set(prev=>({...prev,status:'checking',error:''}));
    try{
      const latest=await window.weave.checkUpdate(kind);
      if(!/^\d+\.\d+\.\d+$/.test(latest.version))throw Error('Invalid GitHub release version');
      set({status:compareVersions(installed,latest.version)<0?'available':'current',latest:latest.version,progress:0,error:''});
    }catch(error){
      const msg=error instanceof Error?error.message:String(error);
      set({status:'unknown',latest:'',progress:0,error:msg});notify(`Update ${kind}: ${msg}`);
    }
  },[connected,firmware,notify]);
  const activateUpdate=async(kind:UpdateKind)=>{
    const state=kind==='app'?appUpdate:firmwareUpdate;
    const set=kind==='app'?setAppUpdate:setFirmwareUpdate;
    if(state.status==='checking'||state.status==='downloading')return;
    if(state.status!=='available'||!state.latest){await checkUpdate(kind);return;}
    if(!installedUpdateVersion(kind)){set(emptyUpdate());return;}
    set(prev=>({...prev,status:'downloading',progress:0,error:''}));
    try{
      const location=await window.weave.downloadUpdate(kind,state.latest);
      notify(`Downloaded ${kind} v${state.latest} to ${location}. Install it, then check again.`);
      // Only a newly observed installed version can make the button show ✓.
      set(prev=>({...prev,status:'available',progress:0}));
    }catch(error){
      const msg=error instanceof Error?error.message:String(error);
      set({status:'unknown',latest:'',progress:0,error:msg});notify(`Update ${kind}: ${msg}`);
    }
  };
  useEffect(()=>window.weave.onUpdateProgress(({kind,percent})=>{
    const set=kind==='app'?setAppUpdate:setFirmwareUpdate;
    set(prev=>prev.status==='downloading'?{...prev,progress:percent}:prev);
  }),[]);
  useEffect(()=>{
    let active=true;
    void window.weave.cachedImage('flipper').then(data=>{if(active)setFlipperLogo(data);}).catch(()=>{});
    void window.weave.cachedImage('avatar').then(data=>{if(active)setAuthorAvatar(data);}).catch(()=>{});
    return()=>{active=false;};
  },[]);
  useEffect(()=>{
    if(page!=='Settings')return;
    void checkUpdate('app');
    if(connected)void checkUpdate('firmware');else setFirmwareUpdate(emptyUpdate());
  },[page,connected,firmware,checkUpdate]);
  const updateLabel=(kind:UpdateKind,state:UpdateView)=>{
    const installed=installedUpdateVersion(kind);
    if(!installed)return kind==='firmware'?'Flipper disconnected':'Version unavailable';
    return state.latest&&compareVersions(installed,state.latest)<0
      ?`v${installed} → v${state.latest}`:`v${installed}`;
  };
  const updateSymbol=(state:UpdateView)=>state.status==='available'?'↻':
    state.status==='current'?'✓':state.status==='downloading'?`↓ ${state.progress}%`:'?';
  // A connected USB CDC/RPC link outranks a previously authorized BLE device.
  // Bluetooth pairing itself needs a user gesture; after the first pairing,
  // getDevices() / rememberedBle can reconnect without showing the chooser.
  async function connectUsb(id:string){
    if(linkBusy.current||!service||connectionRef.current==='USB'||bridge.current?.isBusy)return;
    linkBusy.current=true;setConnecting(true);
    let candidate:FlipperBridge|null=null;
    try{
      await window.weave.connectUsb(id);
      await window.weave.sendUsb(Array.from(new TextEncoder().encode('\rstart_rpc_session\r')));
      await sleep(350);
      candidate=createBridge(data=>window.weave.sendUsb(Array.from(data)),512,false);
      usbCandidate.current=candidate;
      // Do not abandon a working BLE link until the new RPC port answers.
      const info=await candidate.deviceInfo();
      const old=bridge.current,oldBle=bleRef.current;
      // Never abort a file transfer or a running command just to switch links.
      const deadline=Date.now()+6500;
      while(old?.isBusy&&Date.now()<deadline)await sleep(150);
      if(old?.isBusy)throw Error('Active Bluetooth transfer; USB switch deferred');
      bridge.current=candidate;usbCandidate.current=null;
      usbRef.current=true;connectionRef.current='USB';setTransport('USB');
      if(old&&old!==candidate)old.close();
      if(oldBle){rememberedBle.current=oldBle;bleRef.current=null;try{oldBle.gatt?.disconnect();}catch{}}
      lastUsbFailure.current=null;
      await initialize(candidate,info);
      notify('USB connected automatically');
    }catch(e){
      candidate?.close();usbCandidate.current=null;
      pendingUsbClose.current=true;
      try{await window.weave.disconnectUsb();}catch{}
      pendingUsbClose.current=false;
      lastUsbFailure.current={id,at:Date.now()};
      if(connectionRef.current==='USB'&&bridge.current===candidate)await detach();
      notify('USB not ready; '+(connectionRef.current==='Bluetooth'?'Bluetooth retained':'retrying')+': '+String(e));
    }finally{linkBusy.current=false;setConnecting(false);}
  }
  async function connectBle(pair=false){
    if(linkBusy.current||!service||connectionRef.current)return;
    if(!navigator.bluetooth){if(pair)notify('Bluetooth not supported in this Electron configuration');return;}
    linkBusy.current=true;setConnecting(true);
    let device:any;
    try{
      if(pair){
        device=await navigator.bluetooth.requestDevice({filters:BLE_PAIR_FILTERS,optionalServices:[SERIAL,'battery_service','device_information']});
        rememberedBle.current=device;
      }else{
        const known=typeof navigator.bluetooth.getDevices==='function'?await navigator.bluetooth.getDevices():[];
        device=rememberedBle.current||known.find((d:any)=>/^(flipper|weave)/i.test(d.name||''))||
          (known.length===1?known[0]:undefined);
        if(!device)return; // The first pairing must be explicitly authorized.
      }
      const server=await device.gatt.connect();bleRef.current=device;
      const serial=await server.getPrimaryService(SERIAL);
      const tx=await serial.getCharacteristic(TX),rx=await serial.getCharacteristic(RX),flow=await serial.getCharacteristic(FLOW);
      let credits=0;
      const refreshCredits=(raw:DataView)=>{if(raw.byteLength>=4)credits=raw.getUint32(0,false);};
      let client:FlipperBridge;
      const incoming=(event:any)=>{const value=event.target.value as DataView;client?.receive(new Uint8Array(value.buffer,value.byteOffset,value.byteLength));};
      await tx.startNotifications();tx.addEventListener('characteristicvaluechanged',incoming);
      await flow.startNotifications();flow.addEventListener('characteristicvaluechanged',(event:any)=>refreshCredits(event.target.value));
      try{refreshCredits(await flow.readValue());}catch{}
      let writePayload=120;
      client=createBridge(async data=>{
        for(let offset=0;offset<data.length;){
          const started=Date.now();while(credits<=0){if(Date.now()-started>9000)throw Error('Flipper BLE flow credits timed out');await sleep(50);}
          const n=Math.min(writePayload,credits,data.length-offset);
          try{await rx.writeValueWithResponse(data.subarray(offset,offset+n));}
          catch(error){if(writePayload>20){writePayload=20;continue;}throw error;}
          credits-=n;offset+=n;await sleep(10);
        }
      },128);
      device.addEventListener('gattserverdisconnected',()=>{
        if(bridge.current===client&&connectionRef.current==='Bluetooth')void detach();
      });
      connectionRef.current='Bluetooth';setTransport('Bluetooth');
      try{
        const bs=await server.getPrimaryService('battery_service');const level=await bs.getCharacteristic('battery_level');
        const val=await level.readValue();setBattery(val.getUint8(0));
      }catch{}
      await initialize(client);
      lastBleFailure.current=0;notify('Bluetooth connected automatically');
    }catch(e){
      lastBleFailure.current=Date.now();
      notify('Bluetooth connection: '+String(e));
      if(connectionRef.current==='Bluetooth')await detach();
    }finally{linkBusy.current=false;setConnecting(false);}
  }
  async function pairBle(){
    if(!navigator.bluetooth){notify('Bluetooth not supported');return;}
    if(connectionRef.current!=='USB'){await connectBle(true);return;}
    // Pair without dropping an already working USB connection.
    try{rememberedBle.current=await navigator.bluetooth.requestDevice({
      filters:BLE_PAIR_FILTERS,optionalServices:[SERIAL,'battery_service','device_information']
    });notify('Bluetooth paired; automatic fallback enabled');}
    catch(e){notify('Bluetooth pairing: '+String(e));}
  }
  useEffect(()=>{
    if(!service)return;
    let active=true;
    const probe=async()=>{
      if(!active||linkBusy.current)return;
      try{
        const ports=await window.weave.scanUsb();
        if(!active)return;
        setDevices(ports);
        const candidate=ports[0]?.id;
        if(candidate&&connectionRef.current!=='USB'&&
           (!lastUsbFailure.current||lastUsbFailure.current.id!==candidate||Date.now()-lastUsbFailure.current.at>12000)){
          await connectUsb(candidate);
        }else if(!connectionRef.current&&Date.now()-lastBleFailure.current>10000){
          await connectBle();
        }
      }catch(error){if(!connectionRef.current)notify('Connection scan: '+String(error));}
    };
    void probe();const timer=setInterval(()=>void probe(),2500);
    return()=>{active=false;clearInterval(timer);};
  },[service]);
  async function press(key:number){if(!bridge.current)return;keyChain.current=keyChain.current.then(()=>withBridge().pressKey(key)).catch(report);await keyChain.current;}
  async function capture(){if(!latestFrame.current){notify('No Flipper display frame available');return;}
    const off=document.createElement('canvas');off.width=128;off.height=64;const cx=off.getContext('2d');if(!cx)return;
    cx.putImageData(new ImageData(screenPixels(latestFrame.current),128,64),0,0);
    const data=atob(off.toDataURL('image/png').split(',')[1]);await saveBytes('flipper-display.png',Uint8Array.from(data,c=>c.charCodeAt(0)));
  }
  const refreshFiles=useCallback(async(path:string)=>{
    const client=bridge.current;if(!client)return;
    setFileBusy(true);try{const items=await client.list(path);setEntries(items);setSelectedFiles([]);setFilesMessage(`${items.length} items · ${path}`);}catch(e){report(e);}finally{setFileBusy(false);}
  },[]);
  useEffect(()=>{if(connected&&page==='Files')void refreshFiles(filePath);},[connected,page,filePath,refreshFiles]);
  const refreshInstalled=useCallback(async()=>{
    if(!bridge.current)return;setAppsBusy(true);try{setInstalled(await bridge.current.installed());}
    catch(e){report(e);}finally{setAppsBusy(false);}
  },[]);
  useEffect(()=>{if(connected&&page==='Apps'&&appMode!=='Shop')void refreshInstalled();},[connected,page,appMode,refreshInstalled]);
  useEffect(()=>{if(page!=='Apps'||appDetails)return;
    let active=true;
    const load=async()=>{
      setCatalogBusy(true);setCatalogError('');
      try{await fetchCompleteCatalog(items=>{if(active)setCatalog(items);},()=>active);}
      catch(e){if(active)setCatalogError(String(e));}
      finally{if(active)setCatalogBusy(false);}
    };
    void load();return()=>{active=false;};
  },[page,catalogRefresh]);
  useEffect(()=>{if(!appDetails||page!=='Apps')return;
    let active=true;setDetailBusy(true);
    void window.weave.catalogDetail(appDetails.id).then(raw=>{
      if(!active)return;const details=parseCatalog([raw])[0];
      if(details)setAppDetails(prev=>prev?.id===details.id?{...prev,...details,
        icon:details.icon||prev.icon,description:details.description||prev.description,
        archiveId:details.archiveId||prev.archiveId,sourceUrl:details.sourceUrl||prev.sourceUrl,descriptionUrl:details.descriptionUrl||prev.descriptionUrl,
        manifestUrl:details.manifestUrl||prev.manifestUrl,
        screenshots:details.screenshots?.length?details.screenshots:prev.screenshots}:prev);
    }).catch(e=>notify('App details: '+String(e))).finally(()=>{if(active)setDetailBusy(false);});
    return()=>{active=false;};
  },[page,appDetails?.id]);
  async function fileAction(kind:string){
    const client=withBridge(),selection=entries.filter(e=>selectedFiles.includes(e.name));
    if(kind==='Copy'||kind==='Move'){setClipboard({move:kind==='Move',items:selection.map(e=>({path:filePath+'/'+e.name,directory:e.directory}))});setSelectedFiles([]);notify(`${selection.length} item(s) in clipboard`);return;}
    if(kind==='Paste'){
      if(!clipboard)return;setFileBusy(true);setProgress(0);
      try{
        const reserved=new Set(entries.map(e=>e.name));let done=0;
        for(const item of clipboard.items){
          if(filePath===item.path||filePath.startsWith(item.path+'/'))throw Error('Cannot paste a folder inside itself');
          const target=filePath+'/'+uniqueName(pathName(item.path),reserved);
          if(clipboard.move){if(item.path.substring(0,item.path.lastIndexOf('/'))===filePath)throw Error('Cannot move an item into the same directory');await client.rename(item.path,target);}
          else await copyRemote(client,item.path,target,item.directory);
          setProgress(Math.floor(++done/clipboard.items.length*100));
        }
        if(clipboard.move)setClipboard(null);notify('Paste confirmed by Flipper');await refreshFiles(filePath);
      }catch(e){report(e);}finally{setFileBusy(false);}return;
    }
    if(kind==='Export'){
      setFileBusy(true);setProgress(0);
      try{
        if(selection.length===1&&!selection[0].directory){const data=await client.read(filePath+'/'+selection[0].name);await saveBytes(selection[0].name,data);}
        else{
          const collected:ArchiveItem[]=[];
          for(const item of selection)await collectRemote(client,filePath+'/'+item.name,item.name,item.directory,collected);
          const title=selection.length===1?selection[0].name+'.zip':'flipper-export.zip';
          if(await saveBytes(title,createZip(collected)))notify('Exported '+title);
        }
      }catch(e){report(e);}finally{setFileBusy(false);}return;
    }
    if(kind==='Import'){
      setFileBusy(true);try{const imported=await window.weave.openFile();if(imported){await client.write(filePath+'/'+imported.name,new Uint8Array(imported.bytes),setProgress);await refreshFiles(filePath);notify('File transferred to Flipper');}}
      catch(e){report(e);}finally{setFileBusy(false);setProgress(0);}return;
    }
    if(kind==='New'){const name=prompt('New folder name');if(!name)return;setFileBusy(true);try{await client.mkdir(filePath+'/'+name);await refreshFiles(filePath);}catch(e){report(e);}finally{setFileBusy(false);}return;}
    if(kind==='Rename'&&selection.length===1){const name=prompt('Rename file or folder',selection[0].name);if(!name)return;setFileBusy(true);try{await client.rename(filePath+'/'+selection[0].name,filePath+'/'+name);await refreshFiles(filePath);}catch(e){report(e);}finally{setFileBusy(false);}return;}
    if(kind==='Delete'&&selection.length){if(!confirm(`Delete ${selection.length} item(s) permanently from the Flipper?`))return;
      setFileBusy(true);try{for(const item of selection)await client.delete(filePath+'/'+item.name,true);await refreshFiles(filePath);notify('Flipper confirmed delete');}catch(e){report(e);}finally{setFileBusy(false);}
    }
  }
  async function collectRemote(client:FlipperBridge,path:string,zipPath:string,folder:boolean,results:ArchiveItem[]){
    if(results.length>4095)throw Error('Maximum 4096 items per export');
    if(folder){results.push({path:zipPath,data:new Uint8Array(),directory:true});
      for(const child of await client.list(path))await collectRemote(client,path+'/'+child.name,zipPath+'/'+child.name,child.directory,results);
    }else results.push({path:zipPath,data:await client.read(path),directory:false});
  }
  async function copyRemote(client:FlipperBridge,from:string,to:string,folder:boolean){
    if(folder){await client.mkdir(to);for(const item of await client.list(from))await copyRemote(client,from+'/'+item.name,to+'/'+item.name,item.directory);}
    else await client.write(to,await client.read(from),setProgress);
  }
  async function cliBootstrap(){
    const client=bridge.current;if(!client)return;
    setTerminalBusy(true);try{
      try{await client.mkdir('/ext/.weave');}catch{} // Already exists on connected Weave firmware.
      await client.write('/ext/.weave/cli.in',new TextEncoder().encode('help\r\n'));
      setCliReady(true);notify('Flipper CLI started over '+transport);
    }catch(e){report(e);setCliReady(false);}finally{setTerminalBusy(false);}
  }
  const pollCli=useCallback(async()=>{
    const client=bridge.current;if(!client||cliPolling.current||!cliReady||page!=='Terminal'||terminalMode!=='CLI')return;
    cliPolling.current=true;
    try{
      const output=await client.read('/ext/.weave/cli.out');
      const prefix=cliPrevious.current;
      let unchanged=output.length>=prefix.length;
      if(unchanged)for(let i=0;i<prefix.length;i++)if(output[i]!==prefix[i]){unchanged=false;break;}
      const fresh=unchanged?output.subarray(prefix.length):output;
      if(fresh.length)setTerminalOutput(prev=>(prev+new TextDecoder().decode(fresh)).slice(-32000));
      cliPrevious.current=output;
    }catch(e){notify('CLI: '+String(e));}finally{cliPolling.current=false;}
  },[cliReady,page,terminalMode,notify]);
  useEffect(()=>{if(page!=='Terminal'||terminalMode!=='CLI'||!connected||cliReady)return;void cliBootstrap();},[page,terminalMode,connected,cliReady]);
  useEffect(()=>{if(!cliReady||page!=='Terminal'||terminalMode!=='CLI')return;void pollCli();const timer=setInterval(()=>void pollCli(),2400);return()=>clearInterval(timer);},[cliReady,page,terminalMode,pollCli]);
  async function submitCli(){const command=terminalCommand.trim();if(!command||!cliReady)return;setTerminalCommand('');setTerminalBusy(true);
    try{await withBridge().write('/ext/.weave/cli.in',new TextEncoder().encode(command+'\r\n'));}
    catch(e){report(e);}finally{setTerminalBusy(false);}
  }
  function appCategory(category:string){
    const allowed=['Tools','Games','USB','GPIO','NFC','RFID','Infrared','Sub-GHz','Bluetooth','Media','Other'];
    const key=category.trim().toLowerCase();
    if(key.includes('usb'))return 'USB';
    if(key.includes('nfc'))return 'NFC';
    if(key.includes('infrared')||key==='ir')return 'Infrared';
    if(key.includes('subghz')||key.includes('sub-ghz')||key.includes('radio'))return 'Sub-GHz';
    if(key.includes('rfid')||key.includes('125khz'))return 'RFID';
    if(key.includes('bluetooth')||key==='ble')return 'Bluetooth';
    if(key.includes('gpio'))return 'GPIO';
    if(key.includes('ibutton')||key.includes('1-wire'))return 'iButton';
    if(key.includes('game'))return 'Games';
    if(key.includes('tool')||key.includes('utilit'))return 'Tools';
    return allowed.find(x=>x.toLowerCase()===key)||'Other';
  }
  function catalogAscii(value:string,max:number){return value.replace(/[\t\r\n]/g,' ').replace(/[^ -~]/g,' ').replace(/ +/g,' ').trim().slice(0,max);}
  async function syncNativeCatalog(client:FlipperBridge){
    if(nativeShopBusy.current||client.isUploading)return;
    nativeShopBusy.current=true;
    try{
      const current=await fetchCompleteCatalog(()=>{},()=>bridge.current===client);
      if(!current.length)return;
      setCatalog(current);
      let data='';
      for(const app of current){
        if(!/^[A-Za-z0-9_-]{2,80}$/.test(app.id))continue;
        const title=catalogAscii(app.title,32),category=catalogAscii(app.category,20)||'Other';
        const description=catalogAscii(app.description,94)||'Available from Flipper catalog';
        if(!title)continue;
        const version=catalogAscii(app.version||'',32)||'-';
        const line=`${app.id}\t${title}\t${category}\t${description}\t-\t${version}\n`;
        if(data.length+line.length>16000)break;data+=line;
      }
      if(data&&bridge.current===client){
        await client.write('/ext/.weave/shop.catalog',new TextEncoder().encode(data));
        notify('Native Flipper Shop synchronized via '+transport);
      }
    }catch(error){notify('Native Shop sync: '+String(error));}
    finally{nativeShopBusy.current=false;}
  }
  // IPC downloads execute in Electron's main process. This listener updates the
  // same live installation job shown on the Flipper and its desktop detail page.
  useEffect(()=>window.weave.onFapProgress(state=>{
    const active=nativeDownloadHandle.current;
    if(active&&state.archiveId===active.archiveId)
      active.update(state.received,state.total,state.percent);
  }),[]);
  async function pollNativeShop(){
    const client=bridge.current;if(!client||nativeShopBusy.current||client.isUploading)return;
    nativeShopBusy.current=true;
    let request='';
    try{request=new TextDecoder().decode(await client.read('/ext/.weave/shop.request')).trim();}
    catch{nativeShopBusy.current=false;return;}
    nativeShopBusy.current=false;
    if(!/^[A-Za-z0-9_-]{2,80}\|\d{1,12}-\d{1,12}$/.test(request)||request===nativeShopRequest.current)return;
    nativeShopRequest.current=request;
    nativeShopBusy.current=true;
    const device=client,alias=request.split('|')[0];
    const started=Date.now();
    setNativeShopStatus({id:alias,title:alias,phase:'waiting',percent:-1,eta:-1});
    let lastReportedPercent=-1,lastReportedTime=0;
    const reportStatus=async(status:'downloading'|'installing'|'done'|'failed',percent=-1,eta=-1,total=0,path='')=>{
      if(bridge.current!==device)return;
      const fields=[request,status,percent<0?'-':String(percent),eta<0?'-':String(Math.min(35999,eta)),
        total>0?String(total):'-',path||'-'];
      await client.write('/ext/.weave/shop.result',new TextEncoder().encode(fields.join('|')),()=>{},false);
    };
    try{
      await reportStatus('downloading');
      const results=parseCatalog(await window.weave.catalog(alias,0));
      const entry=results.find(app=>app.id===alias);
      if(!entry||!entry.archiveId)throw Error('Application is no longer in the official catalog');
      setNativeShopStatus({id:alias,title:entry.title,phase:'downloading',percent:-1,eta:-1});
      nativeDownloadHandle.current={archiveId:entry.archiveId,update:(received,total,percent)=>{
        if(bridge.current!==device)return;
        const elapsed=Date.now()-started;
        const eta=total>received&&received>0&&elapsed>0?Math.round((total-received)*elapsed/received/1000):-1;
        setNativeShopStatus({id:alias,title:entry.title,phase:'downloading',percent,eta});
        if(percent>=0&&(percent>=lastReportedPercent+8||Date.now()-lastReportedTime>1900)){
          lastReportedPercent=percent;lastReportedTime=Date.now();
          void reportStatus('downloading',percent,eta).catch(()=>{});
        }
      }};
      const chosen=await window.weave.downloadFap(entry.archiveId,entry.version,firmwareApi);
      nativeDownloadHandle.current=null;
      if(bridge.current!==device)return;
      const binary=new Uint8Array(chosen.bytes);
      const folder='/ext/apps/'+appCategory(entry.category);
      const path=folder+'/'+entry.id+'.fap';
      // HTTP reached 100%: native Flipper viewer keeps spinning until the
      // transfer phase starts. Do not use upload bytes as download progress.
      setNativeShopStatus({id:alias,title:entry.title,phase:'downloading',percent:100,eta:0});
      await reportStatus('downloading',100,0);
      // Install status is fully acknowledged before starting a large RPC upload.
      await reportStatus('installing',0,-1,binary.length,path);
      setNativeShopStatus({id:alias,title:entry.title,phase:'installing',percent:0,eta:-1});
      try{await client.mkdir(folder);}catch{}
      const transferStart=Date.now();
      await client.write(path,binary,(percent)=>{
        const bounded=Math.min(99,Math.max(0,percent));
        const elapsed=Date.now()-transferStart;
        const eta=bounded>1&&elapsed>0?Math.round((100-bounded)*elapsed/bounded/1000):-1;
        setNativeShopStatus({id:alias,title:entry.title,phase:'installing',percent:bounded,eta});
      });
      const listing=await client.list(folder);
      if(!listing.some(item=>item.name===entry.id+'.fap'&&!item.directory))
        throw Error('Flipper did not report the installed FAP');
      const nativeVersion=catalogAscii(entry.version||'',32);
      try{await rememberFapVersion(client,entry.id,nativeVersion,path);}catch{}
      await reportStatus('done',100,0);
      setNativeShopStatus({id:alias,title:entry.title,phase:'done',percent:100,eta:0});
      notify('Native Shop: FAP transfer confirmed for '+entry.title);
      if(page==='Apps'&&appMode!=='Shop')await refreshInstalled();
    }catch(error){
      nativeDownloadHandle.current=null;
      notify('Native Shop install: '+String(error));
      setNativeShopStatus(old=>({id:alias,title:old?.title||alias,phase:'failed',percent:-1,eta:-1}));
      try{await reportStatus('failed');}catch{}
    }finally{nativeShopBusy.current=false;}
  }
  const stopDesktopRing=()=>{
    if(findingInterval.current!==null){clearInterval(findingInterval.current);findingInterval.current=null;}
    const audio=findingAudio.current;findingAudio.current=null;
    if(audio)void audio.close().catch(()=>{});
    setFindingDevice(false);
  };
  const ringDesktop=async()=>{
    // Continue until the user explicitly closes the dialog or clicks Stop.
    stopDesktopRing();
    const ctx=new AudioContext();findingAudio.current=ctx;
    try{
      await ctx.resume();
      const play=()=>{
        if(findingAudio.current!==ctx)return;
        const oscillator=ctx.createOscillator(),gain=ctx.createGain();
        const now=ctx.currentTime;
        oscillator.type='square';oscillator.frequency.value=880;
        gain.gain.setValueAtTime(0,now);
        gain.gain.linearRampToValueAtTime(0.27,now+0.025);
        gain.gain.setValueAtTime(0.27,now+0.31);
        gain.gain.linearRampToValueAtTime(0,now+0.4);
        oscillator.connect(gain);gain.connect(ctx.destination);
        oscillator.start(now);oscillator.stop(now+0.43);
      };
      play();findingInterval.current=setInterval(play,720);
      setFindingDevice(true);
    }catch(error){stopDesktopRing();notify('Find my Device: '+String(error));}
  };
  useEffect(()=>()=>{if(findingInterval.current!==null)clearInterval(findingInterval.current);
    if(findingAudio.current)void findingAudio.current.close().catch(()=>{});},[]);
  async function pollFindDevice(){
    const client=bridge.current;
    if(!client||findBusy.current||client.isUploading||nativeShopBusy.current)return;
    findBusy.current=true;
    try{
      const payload=new TextDecoder().decode(await client.read('/ext/.weave/find-device.req')).trim();
      if(/^\d{1,12}-\d{1,12}$/.test(payload)&&payload!==lastFindRequest.current){
        lastFindRequest.current=payload;
        notify('Flipper requested Find my Device');
        void ringDesktop();
        try{await client.delete('/ext/.weave/find-device.req',false);}catch{}
      }
    }catch{/* The request is optional until the user chooses Find my Device. */}
    finally{findBusy.current=false;}
  }
  async function pollFirmwareCheck(){
    const client=bridge.current;if(!client||findBusy.current||nativeShopBusy.current||client.isUploading)return;
    findBusy.current=true;
    try{
      const token=new TextDecoder().decode(await client.read('/ext/.weave/fw-check.req')).trim();
      if(!/^\d{1,12}-\d{1,12}$/.test(token)||token===lastFirmwareCheckRequest.current)return;
      lastFirmwareCheckRequest.current=token;
      let result='';
      try{
        const release=await window.weave.checkUpdate('firmware');
        result=token+'|ok|'+release.version;
      }catch(error){result=token+'|error|release unavailable';notify('Firmware check: '+String(error));}
      if(bridge.current===client)await client.write('/ext/.weave/fw-check.result',new TextEncoder().encode(result));
    }catch{/* Firmware check is optional until requested in Settings. */}
    finally{findBusy.current=false;}
  }
  async function findMyFlipper(){
    const client=bridge.current;
    if(!client){notify('Connect the Flipper before locating it');return;}
    if(!window.confirm('Ring your Flipper until OK or BACK is pressed on the device?'))return;
    try{
      const token=String(Date.now());
      await client.write('/ext/.weave/find-flipper.req',new TextEncoder().encode(token));
      notify('Find my Flipper: ring requested');
    }catch(error){report(error);}
  }
  useEffect(()=>{if(!connected)return;const timer=setInterval(()=>{void pollFindDevice();setTimeout(()=>void pollFirmwareCheck(),1550);},4500);void pollFindDevice();return()=>clearInterval(timer);},[connected]);
  useEffect(()=>{if(!connected)return;const timer=setInterval(()=>void pollNativeShop(),12500);return()=>clearInterval(timer);},[connected,firmwareApi,page,appMode]);

  // Explicit opt-in. A 10-second heartbeat lease is maintained over the
  // existing authenticated Flipper Storage RPC; disconnect revokes it.
  useEffect(()=>{
    const client=bridge.current;
    if(!connected||!service||!client)return;
    let cancelled=false, busy=false, sequence=(Date.now()>>>0), eventSequence=(Date.now()>>>0)||1;
    const tick=async()=>{
      if(cancelled||busy||bridge.current!==client||client.isUploading)return;
      busy=true;
      try{
        const kind=connectionRef.current==='Bluetooth'?'BLE':'USB';
        await client.write(NET_LINK,networkHeartbeat(kind,sequence=(sequence+1)>>>0),()=>{},false);
        let packet:Uint8Array|null=null;
        try{packet=await client.read(NET_REQUEST);}catch{/* No pending request. */}
        const request=packet?parseNetworkRequest(packet):null;
        if(request){
          // Remove before processing so a crash cannot replay a completed request.
          try{await client.delete(NET_REQUEST,false);}catch{return;}
          const result=await window.weave.network(request,true);
          if(!cancelled&&bridge.current===client){
            await client.write(NET_RESPONSE,networkReply(request.id,result),()=>{},false);
            if(!cancelled&&bridge.current===client)
              await client.write(NET_READY,networkReady(request.id),()=>{},false);
          }
        }
        if(cancelled||bridge.current!==client)return;
        let markerExists=false;
        try{await client.read(NET_EVENT_READY);markerExists=true;}catch{/* Firmware consumed the previous event. */}
        if(!markerExists){
          const event=await window.weave.networkEvent();
          if(event){
            eventSequence=(eventSequence+1)>>>0;if(!eventSequence)eventSequence=1;
            await client.write(NET_EVENT,networkEvent(eventSequence,event),()=>{},false);
            await client.write(NET_EVENT_READY,networkEventReady(eventSequence),()=>{},false);
          }
        }
      }catch(error){if(!cancelled)notify('Network bridge: '+String(error));}
      finally{busy=false;}
    };
    void tick();
    const timer=setInterval(()=>void tick(),2400);
    return()=>{cancelled=true;clearInterval(timer);void window.weave.networkReset();};
  },[connected,service,notify]);

  async function installApp(entry:CatalogEntry){
    if(!connected||!entry.archiveId){report('A connected Flipper and an official catalog application ID are required');return;}
    setAppTransferBusy(true);setAppTransferProgress(0);
    try{
      const chosen=await window.weave.downloadFap(entry.archiveId,entry.version,firmwareApi);
      if(!chosen.compatible){
        if(!confirm(`Published FAP uses SDK ${chosen.api}; your Flipper reports ${firmwareApi||'unknown'}. The FAP may not run. Install anyway?`))return;
      }
      const folder='/ext/apps/'+appCategory(entry.category);
      const rpc=withBridge();
      try{await rpc.mkdir(folder);}catch{}
      const path=folder+'/'+entry.id+'.fap';
      const binary=new Uint8Array(chosen.bytes);
      await rpc.write(path,binary,setAppTransferProgress);
      const installedFiles=await rpc.list(folder);
      if(!installedFiles.some(item=>item.name===entry.id+'.fap'&&!item.directory))
        throw Error('Flipper did not confirm the installed FAP');
      const installedVersion=catalogAscii(entry.version||'',32);
      try{await rememberFapVersion(rpc,entry.id,installedVersion,path);}catch{}
      await refreshInstalled();
      notify(`FAP transfer confirmed by Flipper: ${entry.title}`);
    }catch(error){report(error);}finally{setAppTransferBusy(false);}
  }
  async function importFap(){
    if(!connected){notify('Connect Flipper first');return;}
    setAppTransferBusy(true);setAppTransferProgress(0);
    try{
      const file=await window.weave.openFile();if(!file)return;
      if(!file.name.toLowerCase().endsWith('.fap')||file.bytes.length<4||
        file.bytes[0]!==127||file.bytes[1]!==69||file.bytes[2]!==76||file.bytes[3]!==70)
        throw Error('Import a valid ELF .fap file');
      const category=appCategory(prompt('FAP category (Tools, Games, USB, NFC, RFID, GPIO, Other)', 'Tools')||'Tools');
      const folder='/ext/apps/'+category;
      const rpc=withBridge();try{await rpc.mkdir(folder);}catch{}
      await rpc.write(folder+'/'+file.name,new Uint8Array(file.bytes),setAppTransferProgress);
      await refreshInstalled();notify('FAP transfer confirmed by Flipper: '+file.name);
    }catch(error){report(error);}finally{setAppTransferBusy(false);}
  }
  async function removeApp(path:string){if(!confirm('Remove '+pathName(path)+' from Flipper? Application data is kept.'))return;
    try{await withBridge().delete(path,false);await refreshInstalled();notify('Flipper confirmed application removal');}catch(e){report(e);}
  }
  const filter=search.trim().toLocaleLowerCase();
  const chosenApps=appMode==='Shop'?catalog.filter(item=>!filter||[item.title,item.id,item.category,item.description].some(value=>value.toLocaleLowerCase().includes(filter))):[];
  const openAppDetail=(entry:CatalogEntry)=>{setGalleryIndex(null);setAppDetails(entry);};
  return <div className={"app-shell page-"+page.toLowerCase()+(page==='Home'?" home-page":"")}><aside className="sidebar">
    <div className="brand"><img src="./weave.png" alt="Weave"/><div><strong>Weave</strong><small>Flipper Companion</small></div></div>
    <nav>{(['Home','Apps','Terminal','Files','Settings'] as Page[]).map((n,i)=><button key={n} className={'nav-item '+(page===n||page==='Info'&&n==='Settings'?'active':'')} onClick={()=>{setPage(n);if(n!=='Apps'){setAppDetails(null);setGalleryIndex(null);}}}><img className="nav-icon" src={iconOf(['ic_nav_home','ic_nav_apps','ic_nav_console','ic_nav_files','ic_nav_settings'][i])} alt=""/>{n}</button>)}</nav>
  </aside><main className="main"><header className="topbar"><div><h1>{page}</h1></div><button className="flipper-indicator" disabled={connected||!service||connecting}
  onClick={()=>void pairBle()} title={connected?`Connected via ${transport}`:'USB connects automatically; click once to authorize Bluetooth as fallback'}><span>Flipper</span><strong className={connected?"ready":""}>O</strong></button></header>
    {page==='Home'&&<section className="home-stack" aria-label="Flipper remote control">
      <div className="home-display"><div className="home-display-label">FLIPPER DISPLAY</div><div className="flipper-screen"><canvas ref={canvas} width="128" height="64" className="lcd"/>{(!connected||!frameReady.current)&&<div className="screen-overlay">{screenStatus}</div>}</div></div>
      <div className="controls"><div className="dpad"><div/><button onClick={()=>void press(0)} aria-label="Up">▲</button><div/><button onClick={()=>void press(3)} aria-label="Left">◀</button><button onClick={()=>void press(4)}>OK</button><button onClick={()=>void press(2)} aria-label="Right">▶</button><div/><button onClick={()=>void press(1)} aria-label="Down">▼</button><div/></div><div className="secondary-actions"><button onClick={()=>void press(5)}>↩ Back</button><button onClick={()=>void capture()}>▣ Screenshot</button></div></div>
      <div className="status-row"><div className="status-card"><img className="status-icon" src={iconOf('ic_home_battery_unknown')} alt=""/><div><b>Battery</b><small>{battery===null?'—':battery+'%'}</small></div></div><div className="status-card"><img className="status-icon" src={iconOf('ic_home_bluetooth')} alt=""/><div><b>Bluetooth</b><small>{transport==='Bluetooth'&&connected?'Connected':'Disconnected'}</small></div></div><div className="status-card"><img className="status-icon" src={iconOf('ic_home_usb')} alt=""/><div><b>USB</b><small>{transport==='USB'&&connected?'Connected':'Disconnected'}</small></div></div></div>
    </section>}
    {page==='Apps'&&<div className="stack apps-page scroll-region">
      {!appDetails&&<><div className="toolbar"><div className="segment">{(['Shop','Installed','Maj'] as const).map(mode=><button key={mode} className={appMode===mode?'selected':''} onClick={()=>{setAppMode(mode);setAppDetails(null);setSearch('');}}>{mode}</button>)}</div><button className="outline" onClick={()=>appMode==='Shop'?setCatalogRefresh(i=>i+1):void refreshInstalled()}>↻ Refresh</button><button className="outline" disabled={!connected||appTransferBusy} onClick={()=>void importFap()}>⇧ Import FAP</button></div>
      <input className="search" placeholder="Search applications…" value={search} onChange={e=>setSearch(e.target.value)}/></>}
      {appTransferBusy&&<div className="progress"><div style={{width:`${appTransferProgress}%`}}/></div>}
      {appMode==='Shop'&&<>{appDetails?<div className="panel app-detail">
        <div className="detail-nav"><button className="outline" onClick={()=>setAppDetails(null)} aria-label="Back to applications">← Back</button></div>
        <div className="detail-hero"><div className="detail-metric"><img className="detail-metric-icon" src={iconOf('ic_nav_settings')} alt=""/><span>Version</span><strong>{appDetails.version||'—'}</strong></div><div className="detail-identity"><AppIcon className="detail-icon" entry={appDetails}/><h2>{appDetails.title}</h2><span>{appDetails.category}</span></div><div className="detail-metric"><img className="detail-metric-icon" src={iconOf('ic_apps_size')} alt=""/><span>Size</span><strong>{appDetails.size?`${(appDetails.size/1024).toFixed(1)} KiB`:'—'}</strong></div></div>
        <div className="detail-actions"><button disabled={!connected||appTransferBusy||!appDetails.archiveId||
          (nativeShopStatus?.id===appDetails.id&&nativeShopStatus.phase!=='done'&&nativeShopStatus.phase!=='failed')}
          onClick={()=>void installApp(appDetails)}>{nativeShopStatus?.id===appDetails.id&&
          (nativeShopStatus.phase==='downloading'||nativeShopStatus.phase==='installing'||nativeShopStatus.phase==='waiting')?
          'Installing…':appTransferBusy?'Installing…':'↓ Install'}</button>{detailBusy&&<span className="muted micro">Loading details…</span>}</div>
        {nativeShopStatus?.id===appDetails.id&&<div className="panel" role="status" aria-live="polite">
          <strong>{nativeShopStatus.phase==='done'?'App installed':nativeShopStatus.phase==='failed'?'Installation failed':
            nativeShopStatus.phase==='installing'?'Transferring to Flipper…':nativeShopStatus.phase==='downloading'?
            'Downloading FAP…':'Waiting for companion…'}</strong>
          {nativeShopStatus.phase!=='done'&&nativeShopStatus.phase!=='failed'&&<>
            <div className="progress" style={{marginTop:8}}><div style={{width:`${Math.max(0,nativeShopStatus.percent)}%`}}/></div>
            <small className="muted">{nativeShopStatus.percent>=0?`${nativeShopStatus.percent}%`:'Measuring…'}
            {nativeShopStatus.eta>0?` · ETA ${Math.floor(nativeShopStatus.eta/60).toString().padStart(2,'0')}:${(nativeShopStatus.eta%60).toString().padStart(2,'0')}`:''}</small>
          </>}
        </div>}
        {!!appDetails.screenshots?.length&&<div className="detail-gallery" aria-label="Screenshots">{appDetails.screenshots.map((url,i)=><button key={url} onClick={()=>setGalleryIndex(i)} aria-label={`Open screenshot ${i+1}`}><img src={url} loading="lazy" alt={`${appDetails.title} screenshot ${i+1}`}/></button>)}</div>}
        <h3 className="detail-heading">Description</h3>
        <div className="markdown-body"><ReactMarkdown remarkPlugins={[remarkGfm]} rehypePlugins={[rehypeRaw,[rehypeSanitize,githubHtmlSchema]]} components={{
          a:({href,children})=>{const safe=catalogAsset(href,appDetails.descriptionUrl)|| (href?.startsWith('https://')?href:undefined);return <a href={safe} onClick={e=>{e.preventDefault();if(safe)void window.weave.openUrl(safe).catch(report);}}>{children}</a>},
          img:({src,alt})=><img className="markdown-image" src={catalogAsset(src,appDetails.descriptionUrl)} alt={alt||''}/>
        }}>{appDetails.description||'No published description available.'}</ReactMarkdown></div>
        <div className="detail-links">{appDetails.sourceUrl&&<button className="detail-link" onClick={()=>void window.weave.openUrl(appDetails.sourceUrl!).catch(report)}><img src={iconOf('ic_link_github')} alt=""/>Repository</button>}{appDetails.manifestUrl&&<button className="detail-link" onClick={()=>void window.weave.openUrl(appDetails.manifestUrl!).catch(report)}><img src={iconOf('ic_link_github')} alt=""/>Manifest</button>}<button className="detail-link" onClick={()=>void window.weave.openUrl('https://lab.flipper.net/apps/'+encodeURIComponent(appDetails.id)).catch(report)}><img src="./weave.png" alt=""/>Flipper Lab</button></div>
        {!appDetails.archiveId&&<p className="muted micro">This application was indexed from the official source manifests. Installation requires a published build verified by the catalog.</p>}
      </div>:<>{catalogBusy&&<p className="muted">Loading the full catalog… {catalog.length} apps</p>}{catalogError&&<p className="warning">{catalogError}</p>}<div className="muted micro">{chosenApps.length} applications{filter?' matching your search':''}</div><div className="app-grid">{chosenApps.map(app=><CatalogTile key={app.id} entry={app} onOpen={openAppDetail}/>)}</div></>}</>}
      {appMode!=='Shop'&&<><div className="muted">{connected?(appsBusy?'Reading installed apps from Flipper…':`${installed.length} verified FAP applications`):'Flipper disconnected'}</div><div className="file-list">{installed.filter(a=>a.name.toLowerCase().includes(search.toLowerCase())).map(app=>{
        const key=normalizeAppName(app.name);
        const matched=catalog.find(entry=>normalizeAppName(entry.id)===key||normalizeAppName(entry.title)===key);
        return <div className="file-row installed-app-row" key={app.path}><AppIcon entry={matched} className="installed-app-icon"/><div><b>{app.name}</b><small>{app.path}</small></div><button className="outline" onClick={()=>void withBridge().launch(app.path).catch(report)}>Launch</button><button className="danger" onClick={()=>void removeApp(app.path)}>Remove</button></div>;})}</div>{appMode==='Maj'&&<p className="muted micro">Updates are shown only when verified installed metadata is available.</p>}</>}
    </div>}
    {page==='Terminal'&&<div className="stack terminal-stack scroll-region"><div className="toolbar"><div className="segment">{(['CLI','LOGS'] as const).map(mode=><button key={mode} className={terminalMode===mode?'selected':''} onClick={()=>setTerminalMode(mode)}>{mode}</button>)}</div><button className="outline" onClick={()=>void navigator.clipboard?.writeText(terminalMode==='CLI'?terminalOutput:logs.join('\n'))}>Copy</button><button className="outline" onClick={()=>void saveBytes('weave-terminal.txt',new TextEncoder().encode(terminalMode==='CLI'?terminalOutput:logs.join('\n')))}>Export</button><button className="outline" onClick={()=>terminalMode==='CLI'?setTerminalOutput(''):setLogs([])}>Clear</button></div>
      <div className="terminal"><pre>{terminalMode==='CLI'?(terminalOutput||'Connect Flipper to open the native CLI…'):logs.join('\n')}</pre>{terminalMode==='CLI'&&<form className="terminal-prompt" onSubmit={e=>{e.preventDefault();void submitCli();}}><span>❯</span><input autoComplete="off" spellCheck={false} aria-label="Flipper command" value={terminalCommand} onChange={e=>setTerminalCommand(e.target.value)} disabled={!cliReady||terminalBusy} placeholder={cliReady?'Type a command and press Enter':'Waiting for the native CLI…'}/></form>}</div>
    </div>}
    {page==='Files'&&<div className="stack files-page scroll-region"><div className="toolbar"><div className="segment"><button className={filesRoot==='/int'?'selected':''} onClick={()=>{setFilesRoot('/int');setFilePath('/int');}}>Internal Storage</button><button className={filesRoot==='/ext'?'selected':''} onClick={()=>{setFilesRoot('/ext');setFilePath('/ext');}}>SD Card</button></div><button className="outline" disabled={!connected||fileBusy} onClick={()=>void refreshFiles(filePath)}>↻ Refresh</button></div>
      <div className="panel files-panel"><div className="file-heading"><div><h2>{filePath}</h2><span className="muted micro">{filesMessage}</span></div></div>
        <div className="file-toolbar">{['New','Import','Export','Delete','Rename','Move','Copy','Paste'].map(kind=><button key={kind} className="outline" disabled={!connected||fileBusy||(['Export','Delete','Move','Copy'].includes(kind)&&!selectedFiles.length)||(kind==='Rename'&&selectedFiles.length!==1)||(kind==='Paste'&&!clipboard)} onClick={()=>void fileAction(kind)}>{kind}</button>)}</div>
        {fileBusy&&<div className="progress"><div style={{width:`${progress}%`}}/></div>}
        <div className="file-list">{filePath!==filesRoot&&<button className="file-row parent-folder-row" onClick={()=>setFilePath(filePath.substring(0,filePath.lastIndexOf('/'))||filesRoot)} aria-label="Go to parent folder"><span className="file-select parent-space"/><img className="file-kind-icon" src={iconOf('ic_files_folder')} alt=""/><span className="filename">..</span><small>Parent folder</small></button>}{entries.map(item=><div className={'file-row '+(selectedFiles.includes(item.name)?'picked':'')} key={item.name}><button className="file-select" onClick={()=>setSelectedFiles(current=>current.includes(item.name)?current.filter(n=>n!==item.name):[...current,item.name])}>{selectedFiles.includes(item.name)?'☑':'☐'}</button><img className="file-kind-icon" src={iconOf(item.directory?'ic_files_folder':'ic_files_document')} alt=""/><button className="filename" onClick={()=>item.directory?setFilePath(filePath+'/'+item.name):setSelectedFiles(current=>current.includes(item.name)?current.filter(n=>n!==item.name):[...current,item.name])}>{item.name}</button><small>{item.directory?'Folder':`${item.size} B`}</small></div>)}</div>
      </div></div>}
    {(page==='Settings'||page==='Info')&&<div className="stack settings-page scroll-region">{page==='Settings'?<><div className="panel settings-list"><label><span><b>Service Weave</b><small>Enable the Flipper connection gateway</small></span><input type="checkbox" checked={service} onChange={e=>{setService(e.target.checked);if(!e.target.checked&&connected)void detach();}}/></label><label><span><b>Start on boot</b><small>Start the desktop companion when you sign in</small></span><input type="checkbox" checked={autostart} onChange={e=>{void window.weave.setStartOnBoot(e.target.checked).then(setAutostart).catch(report);}}/></label><button className="settings-link" disabled={!connected} aria-disabled={!connected} title={connected?"Ring your Flipper until OK / BACK":"Connect a Flipper first"} onClick={()=>void findMyFlipper()}><span className="settings-link-main"><img src={iconOf('ic_settings_find_flipper')} alt="" className="settings-link-icon"/>Find my Flipper</span><span>›</span></button><button className="settings-link" onClick={()=>setPage('Info')}><span className="settings-link-main"><img src={iconOf('ic_settings_info')} alt="" className="settings-link-icon"/>Info</span><span>›</span></button></div><div className="status-row update-status-row">{([['app',appUpdate],['firmware',firmwareUpdate]] as [UpdateKind,UpdateView][]).map(([kind,state])=><div className="status-card update-card" key={kind}>
      <img className="update-logo" src={kind==='app'?'./weave.png':(flipperLogo||'./flipper-fallback.svg')} alt={kind==='app'?'Weave':'qFlipper'} />
      <b>{kind==='app'?'Update App':'Update Firmware'}</b>
      <small>{updateLabel(kind,state)}</small>
      <button className={'update-action '+(state.status==='downloading'?'downloading':'')}
        disabled={state.status==='checking'||state.status==='downloading'}
        onClick={()=>void activateUpdate(kind)}
        aria-label={state.status==='available'?'Download update': 'Check for updates'}
        title={state.error|| (state.status==='current'?'Check again':state.status==='available'?'Download update':'Check for updates')}>
        <span className="update-progress-fill" style={{width:state.status==='downloading'?`${state.progress}%`:'0%'}} />
        <span className="update-action-label">{updateSymbol(state)}</span>
      </button>
    </div>)}</div></>:<div className="panel info"><button className="outline" onClick={()=>setPage('Settings')}>← Back</button><img className="info-logo" src="./weave.png" alt="Weave"/><h2>Weave</h2><p>Network API firmware for Flipper Zero applications.</p><div className="info-story"><h3>From FlipNet</h3><p>FlipNet was the prototype. Weave brings network access into the firmware for installed applications.</p></div><div className="author"><img src={authorAvatar||'./github-fallback.svg'} alt="Nullmess profile"/><div><b>Nullmess</b><small>Project Manager</small></div><button className="author-link" aria-label="GitHub" onClick={()=>void window.weave.openUrl('https://github.com/Nullmess').catch(report)}><img src={iconOf('ic_link_github')} alt=""/>GitHub</button><button className="author-link" aria-label="Ko-fi" onClick={()=>void window.weave.openUrl('https://ko-fi.com/macxzew').catch(report)}><img src={iconOf('ic_settings_kofi')} alt=""/>Ko-fi</button></div></div>}</div>}
    {findingDevice&&<div className="finder-overlay" onClick={stopDesktopRing} role="presentation">
      <div className="finder-dialog" role="dialog" aria-modal="true" aria-label="Find my Device" onClick={event=>event.stopPropagation()}>
        <h2>Find my Device</h2><p>Your computer is ringing.</p>
        <button onClick={stopDesktopRing}>Stop ringing</button>
      </div>
    </div>}
    {galleryIndex!==null&&appDetails?.screenshots?.[galleryIndex]&&<div className="finder-overlay" onClick={()=>setGalleryIndex(null)}><div className="image-viewer" onClick={e=>e.stopPropagation()}><button className="outline" onClick={()=>setGalleryIndex(null)}>Close ×</button><img src={appDetails.screenshots[galleryIndex]} alt="App screenshot"/><div><button className="outline" onClick={()=>setGalleryIndex(i=>Math.max(0,(i||0)-1))} disabled={galleryIndex===0}>←</button><span>{galleryIndex+1} / {appDetails.screenshots.length}</span><button className="outline" onClick={()=>setGalleryIndex(i=>Math.min(appDetails.screenshots!.length-1,(i||0)+1))} disabled={galleryIndex===appDetails.screenshots.length-1}>→</button></div></div></div>}
  </main></div>;
}
createRoot(document.getElementById('root')!).render(<React.StrictMode><App/></React.StrictMode>);
