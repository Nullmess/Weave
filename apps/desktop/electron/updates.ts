import {app, shell, BrowserWindow} from 'electron';
import path from 'node:path';
import {promises as fs} from 'node:fs';

const REPO='Nullmess/Weave';
const RELEASE=`https://api.github.com/repos/${REPO}/releases/latest`;
const HEADERS={'User-Agent':'Weave/1.0.4','Accept':'application/vnd.github+json'};
const MAX_DOWNLOAD=250_000_000;
export type UpdateKind='app'|'firmware';
export type ReleaseAsset={version:string;name:string;size:number};
const platformName=process.platform==='win32'?'windows':process.platform==='darwin'?'macos':'linux';
const imageUrls:Record<string,string>={
  flipper:'https://cdn.flipper.net/qFlipper_macOS_256px_ugly.png',
  avatar:'https://avatars.githubusercontent.com/u/326523721',
};
function validateAssetUrl(url:string):boolean{
  try {const u=new URL(url);return u.protocol==='https:'&&
    (u.hostname==='github.com'||u.hostname==='release-assets.githubusercontent.com'||u.hostname==='objects.githubusercontent.com') &&
    !u.username&&!u.password;}
  catch{return false;}
}
const withTimeout=async(url:string,headers:Record<string,string>,ms:number)=>{
  const controller=new AbortController();const timer=setTimeout(()=>controller.abort(),ms);
  try{return await fetch(url,{headers,signal:controller.signal});}finally{clearTimeout(timer);}
};
async function publishedAsset(kind:UpdateKind){
  const response=await withTimeout(RELEASE,HEADERS,14000);
  if(!response.ok)throw Error(response.status===404?'No public GitHub release found':`GitHub HTTP ${response.status}`);
  const body=await response.text();if(body.length>1_000_000)throw Error('GitHub response too large');
  const release=JSON.parse(body) as {tag_name?:string;assets?:Array<{name?:string;size?:number;browser_download_url?:string}>};
  const version=String(release.tag_name||'').replace(/^v/i,'');
  if(!/^\d+\.\d+\.\d+$/.test(version))throw Error('Release version unavailable');
  const asset=(release.assets||[]).find(a=>{
    const name=String(a.name||'').toLowerCase();
    return kind==='firmware' ? name.endsWith('.tgz')&&name.includes('firmware') :
      name.endsWith('.zip')&&name.includes(platformName)&&name.includes('weave');
  });
  if(!asset||!asset.browser_download_url||!validateAssetUrl(asset.browser_download_url)||
     !asset.browser_download_url.startsWith(`https://github.com/${REPO}/releases/download/`))
    throw Error('Release v'+version+' build is not published yet ('+(kind==='firmware'?'firmware':platformName)+')');
  const size=Number(asset.size);
  if(!Number.isSafeInteger(size)||size<=0||size>MAX_DOWNLOAD)throw Error('Invalid release asset size');
  return {version,name:String(asset.name),size,url:asset.browser_download_url};
}
export async function checkLatest(kind:UpdateKind):Promise<ReleaseAsset>{
  const {version,name,size}=await publishedAsset(kind);return {version,name,size};
}
export async function downloadLatest(kind:UpdateKind,expectedVersion:string,win:BrowserWindow|null):Promise<string>{
  const asset=await publishedAsset(kind);
  if(asset.version!==expectedVersion)throw Error('Release changed; check updates again');
  const response=await withTimeout(asset.url,{'User-Agent':'Weave/1.0.4'},22000);
  if(!response.ok||!response.body||!validateAssetUrl(response.url))throw Error('Release download unavailable');
  if(Number(response.headers.get('content-length')||'0')>MAX_DOWNLOAD)throw Error('Release exceeds size limit');
  const name=path.basename(asset.name);
  if(name!==asset.name||!/^Weave[-_a-zA-Z0-9.]+\.(zip|tgz)$/i.test(name))throw Error('Unexpected download filename');
  const destination=path.join(app.getPath('downloads'),name);
  const part=destination+'.part';
  const file=await fs.open(part,'w');let bytes=0;
  try{
    const reader=response.body.getReader();
    try{
      while(true){
        const {done,value:chunk}=await reader.read();
        if(done)break;
        bytes+=chunk.length;
        if(bytes>MAX_DOWNLOAD)throw Error('Update exceeds size limit');
        await file.write(chunk);
        win?.webContents.send('weave:update:progress',{kind,percent:Math.min(99,Math.round(bytes*100/asset.size))});
      }
    }finally{reader.releaseLock();}
    await file.sync();
  }catch(error){await file.close();await fs.rm(part,{force:true});throw error;}
  await file.close();
  if(bytes<128) {await fs.rm(part,{force:true});throw Error('Invalid update download');}
  const header=await fs.open(part,'r');const magic=Buffer.alloc(4);try{await header.read(magic,0,4,0);}finally{await header.close();}
  if(kind==='firmware' ? magic[0]!==0x1f||magic[1]!==0x8b : magic[0]!==0x50||magic[1]!==0x4b){
    await fs.rm(part,{force:true});throw Error('Unexpected update archive format');
  }
  await fs.rm(destination,{force:true});
  await fs.rename(part,destination);
  win?.webContents.send('weave:update:progress',{kind,percent:100});
  shell.showItemInFolder(destination);
  return destination;
}
const pendingImages=new Map<string,Promise<string>>();
export function cachedImage(key:string):Promise<string>{
  if(!Object.hasOwn(imageUrls,key))return Promise.reject(Error('Unknown image'));
  if(!pendingImages.has(key))pendingImages.set(key,loadCachedImage(key).catch(e=>{pendingImages.delete(key);throw e;}));
  return pendingImages.get(key)!;
}
async function loadCachedImage(key:string):Promise<string>{
  const target=path.join(app.getPath('userData'),'images',key+'.png');
  let bytes:Buffer;
  try{bytes=await fs.readFile(target);}catch{
    const url=imageUrls[key];const response=await withTimeout(url,{'User-Agent':'Weave/1.0.4'},14000);
    if(!response.ok||new URL(response.url).hostname!==new URL(url).hostname)throw Error('Image unavailable');
    const data=Buffer.from(await response.arrayBuffer());
    if(data.length>3_000_000||data.length<100)throw Error('Invalid image');
    bytes=data;
    await fs.mkdir(path.dirname(target),{recursive:true});
    await fs.writeFile(target,bytes);
  }
  // GitHub avatars can be JPEG even with an extensionless URL; identify the actual bytes.
  const mime=bytes[0]===0x89&&bytes[1]===0x50?'image/png':
    bytes[0]===0xff&&bytes[1]===0xd8?'image/jpeg':null;
  if(!mime)throw Error('Unsupported image');
  return `data:${mime};base64,${bytes.toString('base64')}`;
}
