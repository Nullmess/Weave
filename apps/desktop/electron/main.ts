import {app, BrowserWindow, ipcMain, session, dialog, shell} from 'electron';
import path from 'node:path';
import {promises as fs} from 'node:fs';
import {SerialPort} from 'serialport';
import {checkLatest,downloadLatest,cachedImage,type UpdateKind} from './updates';
import {getCatalogManifests,getCatalogDetail,getCatalogVisual} from './catalog';
import {NetworkEngine,type NetworkOperation} from './network-engine';
let window: BrowserWindow | null = null;
let port: SerialPort | null = null;
function notify(message: string) { window?.webContents.send('weave:status', message); }
function createWindow(): void {
    window = new BrowserWindow({width:1140,height:790,minWidth:780,minHeight:600,backgroundColor:'#050505',title:'Weave',autoHideMenuBar:true,webPreferences:{preload:path.join(__dirname,'preload.js'),contextIsolation:true,nodeIntegration:false,sandbox:false,backgroundThrottling:false}});
    if (process.env.VITE_DEV_SERVER_URL) window.loadURL(process.env.VITE_DEV_SERVER_URL);
    else window.loadFile(path.join(__dirname,'../dist/index.html'));
    window.webContents.on('select-bluetooth-device', (event,devices,callback) => {
        event.preventDefault();
        // Browser-side pairing filters constrain candidates to the Flipper BLE
        // advertisement service (0x3080..0x3083) or legacy names. Custom
        // hostnames are valid; GATT RPC service is verified after selection.
        const selected = devices.find(d => /^(flipper|weave|flip[_-])/i.test(d.deviceName))
            ?? devices[0];
        callback(selected?.deviceId ?? '');
    });
}
app.whenReady().then(() => {
    // Electron's permission unions vary between releases. Never grant access to
    // unrelated windows or permissions (camera, microphone, location, etc.).
    session.defaultSession.setPermissionCheckHandler((webContents, permission) =>
        webContents === window?.webContents &&
        (String(permission) === 'bluetooth' || String(permission) === 'serial'));
    session.defaultSession.setPermissionRequestHandler((webContents, permission, callback) =>
        callback(webContents === window?.webContents &&
            (String(permission) === 'bluetooth' || String(permission) === 'serial')));
    createWindow();
    app.on('activate',()=>{if(BrowserWindow.getAllWindows().length===0) createWindow();});
});
app.on('window-all-closed',()=>{if(process.platform!=='darwin') app.quit();});
app.on('before-quit',()=>{port?.close(); port=null;});
ipcMain.handle('weave:usb:scan',async()=> {
    const ports=await SerialPort.list();
    const matches=ports.filter(p=> Number.parseInt(p.vendorId??'',16)===0x0483 &&
        Number.parseInt(p.productId??'',16)===0x5740);
    const candidates=await Promise.all(matches.map(async p=>{
        let iface: number|null=null;
        if(process.platform==='linux'){
            try{
                const raw=await fs.readFile(path.join('/sys/class/tty',path.basename(p.path),
                    'device','bInterfaceNumber'),'utf8');
                iface=Number.parseInt(raw.trim(),16);
            }catch{/* Some udev setups do not expose this sysfs attribute. */}
        }
        return {port:p,iface};
    }));
    // Flipper exposes CDC0 (CLI/RPC) and often CDC1 (logs). A logs port can
    // open successfully while never answering start_rpc_session.
    const usable=candidates.filter(item=>item.iface===null||item.iface===0);
    usable.sort((a,b)=>(a.iface===0?-1:0)-(b.iface===0?-1:0));
    return usable.map(({port:p})=>({id:p.path,label:p.manufacturer ?
        `${p.manufacturer} (${p.path})` : p.path}));
});
ipcMain.handle('weave:usb:connect', async(_,identifier:unknown) => {
    if(typeof identifier!=='string' || identifier.length>256) throw Error('Invalid port');
    // The stock Flipper USB CDC CLI carries start_rpc_session; it has no dedicated WV1 interface.
    const discovered = (await SerialPort.list()).filter(p => Number.parseInt(p.vendorId??'',16)===0x0483 && Number.parseInt(p.productId??'',16)===0x5740);
    if(!discovered.some(p=>p.path===identifier)) throw Error('No Flipper Zero USB CDC port');
    if(port?.isOpen) port.close();
    port = new SerialPort({path:identifier,baudRate:115200,autoOpen:false});
    await new Promise<void>((resolve,reject)=>port!.open(e=>e?reject(e):resolve()));
    port.on('data',(data:Buffer)=>window?.webContents.send('weave:usb:data',Array.from(data)));
    port.on('close',()=>notify('USB disconnected'));
    notify('Flipper USB CDC connected');
    return true;
});
ipcMain.handle('weave:usb:send',async(_,data:unknown)=> {
    if(!port?.isOpen || !Array.isArray(data) || data.length>1024 || data.some(v=>!Number.isInteger(v)||v<0||v>255)) throw Error('Invalid USB frame');
    await new Promise<void>((resolve,reject)=>port!.write(Buffer.from(data),e=>e?reject(e):resolve()));
});
ipcMain.handle('weave:usb:close',async()=>{ if(port?.isOpen) await new Promise<void>(r=>port!.close(()=>r())); port=null; });
const networkEngine=new NetworkEngine();
ipcMain.handle('weave:network:reset',()=>{networkEngine.reset();});
ipcMain.handle('weave:network:event',()=>networkEngine.nextEvent());
ipcMain.handle('weave:network',async(_,raw:unknown,approved:unknown)=>{
    if(!raw||typeof raw!=='object')return {status:3,code:0,data:new Uint8Array()};
    const req=raw as NetworkOperation;
    if(typeof req.id!=='string'||typeof req.verb!=='string'||typeof req.target!=='string'||
       !(req.data instanceof Uint8Array))return {status:3,code:0,data:new Uint8Array()};
    return networkEngine.execute(req,approved===true);
});
app.on('before-quit',()=>networkEngine.reset());

// The OS save picker is used for one file or a verified ZIP. Electron never
// accepts an arbitrary destination path from the untrusted renderer.
ipcMain.handle('weave:file:save',async(_event,name:unknown,data:unknown)=>{
    if(typeof name!=='string'||name.length>150||name.includes('/')||name.includes('\\')||
        !Array.isArray(data)||data.length>32_000_000||
        data.some(v=>!Number.isInteger(v)||v<0||v>255))throw Error('Invalid export payload');
    const {canceled,filePath}=await dialog.showSaveDialog(window!,{
        defaultPath:name,properties:['createDirectory','showOverwriteConfirmation'],
        filters:name.endsWith('.zip')?[{name:'ZIP archive',extensions:['zip']}]:[{name:'All files',extensions:['*']}],
    });
    if(canceled||!filePath)return false;
    await fs.writeFile(filePath,Buffer.from(data));
    return true;
});
ipcMain.handle('weave:file:open',async()=>{
    const result=await dialog.showOpenDialog(window!,{properties:['openFile']});
    if(result.canceled||!result.filePaths.length)return null;
    const name=path.basename(result.filePaths[0]);
    const stat=await fs.stat(result.filePaths[0]);
    if(stat.size>2_000_000||stat.size===0)throw Error('File must be 1 byte to 2 MiB');
    return {name,bytes:Array.from(await fs.readFile(result.filePaths[0]))};
});
ipcMain.handle('weave:catalog',async(_event,query:unknown,offset:unknown)=>{
    if(typeof query!=='string'||query.length>90||typeof offset!=='number'||!Number.isInteger(offset)||offset<0||offset>5000)
        throw Error('Invalid catalog query');
    const url=new URL('https://catalog.flipperzero.one/api/v0/application');
    url.searchParams.set('limit','100');url.searchParams.set('offset',String(offset));
    if(query)url.searchParams.set('search',query);
    const controller=new AbortController();
    const timeout=setTimeout(()=>controller.abort(),12000);
    try {
        const response=await fetch(url,{signal:controller.signal,headers:{'Accept':'application/json','User-Agent':'Weave/1.0.0'}});
        if(!response.ok)throw Error('Catalog HTTP '+response.status);
        const body=await response.text();
        if(body.length>5_000_000)throw Error('Catalog response too large');
        return JSON.parse(body);
    }finally{clearTimeout(timeout);}
});
ipcMain.handle('weave:catalog:manifests',async()=>getCatalogManifests());
ipcMain.handle('weave:catalog:visual',async(_event,alias:unknown)=>{
    if(typeof alias!=='string'||!(/^[\w-]{2,100}$/).test(alias))throw Error('Invalid catalog alias');
    return getCatalogVisual(alias);
});
ipcMain.handle('weave:catalog:detail',async(_event,alias:unknown)=>{
    if(typeof alias!=='string'||!(/^[\w-]{2,100}$/).test(alias))throw Error('Invalid catalog alias');
    return getCatalogDetail(alias);
});
ipcMain.handle('weave:open-url',async(_event,url:unknown)=>{
    if(typeof url!=='string'||url.length>1024)throw Error('Invalid URL');
    const parsed=new URL(url);
    if(parsed.protocol!=='https:'||parsed.username||parsed.password||!parsed.hostname)
        throw Error('External link not allowed');
    await shell.openExternal(parsed.toString());
});
ipcMain.handle('weave:login-item',async(_event,enabled:unknown)=>{
    if(typeof enabled!=='boolean')throw Error('Invalid setting');
    if(process.platform==='linux'){
        const location=path.join(app.getPath('home'),'.config','autostart','weave.desktop');
        if(!enabled){await fs.rm(location,{force:true});return false;}
        if(!app.isPackaged)throw Error('Start on boot requires the packaged desktop app');
        const executable=process.execPath.replaceAll('\\','\\\\').replaceAll('"','\\"');
        await fs.mkdir(path.dirname(location),{recursive:true});
        await fs.writeFile(location,`[Desktop Entry]\nType=Application\nName=Weave\nExec="${executable}"\nTerminal=false\nX-GNOME-Autostart-enabled=true\n`,'utf8');
        return true;
    }
    if(app.isPackaged)app.setLoginItemSettings({openAtLogin:enabled});
    return app.getLoginItemSettings().openAtLogin;
});
// Resolve only FAP links actually published on the official Flipper catalog.
// Never invent an API version or a download URL from an application alias.
ipcMain.handle('weave:catalog:fap',async(event,archiveId:unknown,version:unknown,firmwareApi:unknown)=>{
    if(typeof archiveId!=='string'||!/^[a-f0-9]{24}$/i.test(archiveId)||
       typeof version!=='string'||version.length>80||typeof firmwareApi!=='string'||
       !/^(\d+\.\d+)?$/.test(firmwareApi))throw Error('Invalid application build request');
    const base='https://catalog.flipperzero.one';
    const controller=new AbortController(), timer=setTimeout(()=>controller.abort(),16000);
    let paths:string[]=[];
    try{
        const page=await fetch(`${base}/application/${archiveId}/page`,{signal:controller.signal});
        if(!page.ok)throw Error('Official catalog page unavailable');
        const html=await page.text();if(html.length>2_000_000)throw Error('Catalog page is too large');
        const expression=/\/api\/v0\/1\/application\/version\/[a-f0-9]{24}\/build\/f7\/(\d+)\.(\d+)/ig;
        let match:RegExpExecArray|null;
        while((match=expression.exec(html))!==null && paths.length<100){
            if(!paths.includes(match[0]))paths.push(match[0]);
        }
    }finally{clearTimeout(timer);}
    if(!paths.length)throw Error('No published FAP builds listed by Flipper Lab');
    const target=firmwareApi.split('.').map(Number);
    const score=(url:string)=>{
        const tail=url.match(/\/build\/f7\/(\d+)\.(\d+)$/);
        if(!tail)return -Infinity;
        const major=Number(tail[1]),minor=Number(tail[2]);
        if(target.length===2&&major===target[0]&&minor<=target[1])return 100000+minor;
        return major*1000+minor;
    };
    paths.sort((a,b)=>score(b)-score(a));
    for(const candidate of paths){
        const downloadController=new AbortController(),timeout=setTimeout(()=>downloadController.abort(),14000);
        try{
            const response=await fetch(base+candidate,{signal:downloadController.signal});
            if(!response.ok)continue;
            const actual=new URL(response.url);
            if(actual.protocol!=='https:'||!['catalog.flipperzero.one','cdn.flipperzero.one'].includes(actual.hostname))continue;
            const declared=Number(response.headers.get('content-length')||'0');
            if(declared>2_000_000)continue;
            // Stream actual received bytes for UI and the native Flipper Shop.
            // No fabricated percentage if Content-Length is missing.
            const parts:Buffer[]=[];
            let received=0,lastUpdate=0;
            if(!response.body)continue;
            const reader=response.body.getReader();
            for(;;){
                const {value,done}=await reader.read();if(done)break;
                if(!value)continue;
                received+=value.byteLength;
                if(received>2_000_000)throw Error('FAP exceeds size limit');
                parts.push(Buffer.from(value));
                const now=Date.now();
                if(now-lastUpdate>350||declared>0&&received>=declared){
                    lastUpdate=now;
                    event.sender.send('weave:catalog:fap:progress',{
                        archiveId,received,total:declared>0?declared:-1,
                        percent:declared>0?Math.min(99,Math.round(received*100/declared)):-1});
                }
            }
            const bytes=new Uint8Array(Buffer.concat(parts,received));
            if(bytes.length<4||bytes.length>2_000_000||bytes[0]!==127||bytes[1]!==69||bytes[2]!==76||bytes[3]!==70)continue;
            const sdk=candidate.match(/\/build\/f7\/(\d+\.\d+)$/)?.[1]||'';
            return {bytes:Array.from(bytes),api:sdk,compatible:firmwareApi!==''&&sdk===firmwareApi};
        }catch{}finally{clearTimeout(timeout);}
    }
    throw Error('No published FAP binary could be downloaded');
});

// Only published assets from the configured GitHub Release may be downloaded.
ipcMain.handle('weave:updates:check',async(_event,kind:unknown)=>{
    if(kind!=='app'&&kind!=='firmware')throw Error('Invalid update type');
    return checkLatest(kind);
});
ipcMain.handle('weave:updates:download',async(_event,kind:unknown,version:unknown)=>{
    if((kind!=='app'&&kind!=='firmware')||typeof version!=='string'||!/^\d+\.\d+\.\d+$/.test(version))
        throw Error('Invalid update request');
    return downloadLatest(kind as UpdateKind,version,window);
});
ipcMain.handle('weave:image:cached',async(_event,key:unknown)=>{
    if(key!=='avatar'&&key!=='flipper')throw Error('Unknown image');
    return cachedImage(key);
});
