import { contextBridge, ipcRenderer } from 'electron';
const api = {
    scanUsb: () => ipcRenderer.invoke('weave:usb:scan') as Promise<Array<{id:string;label:string}>>,
    connectUsb: (id:string) => ipcRenderer.invoke('weave:usb:connect', id) as Promise<boolean>,
    sendUsb: (frame:number[]) => ipcRenderer.invoke('weave:usb:send', frame) as Promise<void>,
    disconnectUsb: () => ipcRenderer.invoke('weave:usb:close') as Promise<void>,
    network: (request:{id:string;verb:string;target:string;data:Uint8Array},approved:boolean) => ipcRenderer.invoke('weave:network',request,approved) as Promise<{status:number;code:number;data:Uint8Array}>,
    networkReset: () => ipcRenderer.invoke('weave:network:reset') as Promise<void>,
    onUsb: (callback:(frame:number[])=>void) => { const listener = (_:Electron.IpcRendererEvent,data:number[])=>callback(data); ipcRenderer.on('weave:usb:data',listener); return () => ipcRenderer.off('weave:usb:data',listener); },
    onStatus: (callback:(status:string)=>void) => { const listener = (_:Electron.IpcRendererEvent,status:string)=>callback(status); ipcRenderer.on('weave:status',listener); return () => ipcRenderer.off('weave:status',listener); },
};
// File and catalog operations use narrow, validated Electron IPC routes.
Object.assign(api, {
    saveFile: (name:string, data:number[]) => ipcRenderer.invoke('weave:file:save',name,data) as Promise<boolean>,
    openFile: () => ipcRenderer.invoke('weave:file:open') as Promise<{name:string;bytes:number[]}|null>,
    catalog: (query:string,offset:number) => ipcRenderer.invoke('weave:catalog',query,offset) as Promise<unknown>,
    catalogManifests: () => ipcRenderer.invoke('weave:catalog:manifests') as Promise<unknown>,
    catalogDetail: (alias:string) => ipcRenderer.invoke('weave:catalog:detail',alias) as Promise<unknown>,
    catalogVisual: (alias:string) => ipcRenderer.invoke('weave:catalog:visual',alias) as Promise<unknown>,
    openUrl: (url:string) => ipcRenderer.invoke('weave:open-url',url) as Promise<void>,
    setStartOnBoot: (enabled:boolean) => ipcRenderer.invoke('weave:login-item',enabled) as Promise<boolean>,
    checkUpdate: (kind:'app'|'firmware') => ipcRenderer.invoke('weave:updates:check',kind),
    downloadUpdate: (kind:'app'|'firmware',version:string) => ipcRenderer.invoke('weave:updates:download',kind,version),
    cachedImage: (key:'flipper'|'avatar') => ipcRenderer.invoke('weave:image:cached',key),
    onUpdateProgress: (callback:(data:{kind:'app'|'firmware';percent:number})=>void) => {
        const listener = (_:Electron.IpcRendererEvent,data:{kind:'app'|'firmware';percent:number})=>callback(data);
        ipcRenderer.on('weave:update:progress',listener);
        return () => ipcRenderer.off('weave:update:progress',listener);
    },
    onFapProgress: (callback:(state:{archiveId:string;received:number;total:number;percent:number})=>void) => {
        const listener = (_:Electron.IpcRendererEvent,state:{archiveId:string;received:number;total:number;percent:number})=>callback(state);
        ipcRenderer.on('weave:catalog:fap:progress',listener);
        return () => ipcRenderer.off('weave:catalog:fap:progress',listener);
    },
    downloadFap: (archiveId:string,version:string,api:string) => ipcRenderer.invoke('weave:catalog:fap',archiveId,version,api) as Promise<{bytes:number[];api:string;compatible:boolean}>,
});

contextBridge.exposeInMainWorld('weave', api);
