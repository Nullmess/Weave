/// <reference types="vite/client" />
interface WeaveApi {
 checkUpdate(kind:'app'|'firmware'):Promise<{version:string;name:string;size:number}>;
 downloadUpdate(kind:'app'|'firmware',version:string):Promise<string>;
 cachedImage(key:'flipper'|'avatar'):Promise<string>;
 onUpdateProgress(callback:(data:{kind:'app'|'firmware';percent:number})=>void):()=>void;
 scanUsb():Promise<Array<{id:string;label:string}>>; connectUsb(id:string):Promise<boolean>; sendUsb(frame:number[]):Promise<void>; disconnectUsb():Promise<void>;
 network(request:{id:string;verb:string;target:string;data:Uint8Array},approved:boolean):Promise<{status:number;code:number;data:Uint8Array}>; networkReset():Promise<void>; networkEvent():Promise<{kind:'TCP'|'UDP'|'WS';session:number;data:Uint8Array}|null>; onUsb(cb:(data:number[])=>void):()=>void; onStatus(cb:(status:string)=>void):()=>void;
 saveFile(name:string,data:number[]):Promise<boolean>; openFile():Promise<{name:string;bytes:number[]}|null>;
 catalog(query:string,offset:number):Promise<unknown>;catalogManifests():Promise<unknown>;catalogDetail(alias:string):Promise<unknown>;catalogVisual(alias:string):Promise<unknown>;openUrl(url:string):Promise<void>;setStartOnBoot(enabled:boolean):Promise<boolean>;
 onFapProgress(cb:(state:{archiveId:string;received:number;total:number;percent:number})=>void):()=>void;
 downloadFap(archiveId:string,version:string,api:string):Promise<{bytes:number[];api:string;compatible:boolean}>;
}
interface Window {weave:WeaveApi}
interface Navigator { bluetooth?: {requestDevice(options:object):Promise<any>;getDevices?():Promise<any[]>} }
