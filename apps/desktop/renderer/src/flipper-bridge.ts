import {RPC,Framer,deviceInfoPair,deviceInfoRequest,listRequest,readRequest,writeRequest,
    guiStream,guiKey,mkdirRequest,renameRequest,deleteRequest,appLaunch,parseListing,parseRead,parseScreen,
    type Reply,type FileItem} from './flipper-rpc';

export type SendBytes=(packet:Uint8Array)=>Promise<void>;
const pause=(ms:number)=>new Promise<void>(resolve=>setTimeout(resolve,ms));
const safePath=(p:string)=>/^(\/ext|\/int)(?:\/[\w. ()-]{1,150})*$/.test(p)&&!p.split('/').includes('..');

export class FlipperBridge {
    private id=1;
    private assembler=new Framer();
    private pending=new Map<number,{resolve:(chunks:Reply[])=>void;reject:(reason:Error)=>void;timer:ReturnType<typeof setTimeout>;chunks:Reply[]}>();
    private sending=Promise.resolve();
    private uploadChain=Promise.resolve();
    private uploading=false;
    get isUploading(){return this.uploading;}
    get isBusy(){return this.uploading||this.pending.size>0;}
    onScreen:(screen:Uint8Array)=>void=()=>{};
    onWarning:(message:string)=>void=()=>{};
    private closed=false;
    constructor(private transmit:SendBytes,private maxChunk=512){}
    receive(raw:Uint8Array){
        if(this.closed)return;
        try{
            for(const packet of this.assembler.push(raw)){
                if(packet.type===RPC.GUI_FRAME){
                    try {const frame=parseScreen(packet.content);if(frame.length===1024)this.onScreen(frame);}catch{this.onWarning('Invalid Flipper display frame');}
                    continue;
                }
                const pending=this.pending.get(packet.id);
                if(!pending)continue;
                if(packet.status!==0){this.pending.delete(packet.id);clearTimeout(pending.timer);pending.reject(new Error('Flipper RPC status '+packet.status));continue;}
                pending.chunks.push(packet);
                if(!packet.hasNext){this.pending.delete(packet.id);clearTimeout(pending.timer);pending.resolve(pending.chunks);}
            }
        }catch(error){this.onWarning('RPC decode failed: '+String(error));this.assembler.clear();}
    }
    resetFramer(){this.assembler.clear();}
    close(){this.closed=true;this.assembler.clear();for(const item of this.pending.values()){
        clearTimeout(item.timer);item.reject(new Error('Flipper disconnected'));
    }this.pending.clear();}
    private send(packet:Uint8Array):Promise<void>{
        const current=this.sending.then(()=>this.transmit(packet));
        this.sending=current.catch(()=>{});
        return current;
    }
    private call(build:(id:number)=>Uint8Array,timeout=14000):Promise<Reply[]>{
        if(this.closed)return Promise.reject(new Error('Flipper disconnected'));
        const id=this.id++;
        return new Promise((resolve,reject)=>{
            const timer=setTimeout(()=>{this.pending.delete(id);reject(new Error('Flipper RPC response timed out'));},timeout);
            this.pending.set(id,{resolve,reject,timer,chunks:[]});
            this.send(build(id)).catch(error=>{const item=this.pending.get(id);if(item){this.pending.delete(id);clearTimeout(item.timer);reject(error);}});
        });
    }
    async startScreen(){await this.call(id=>guiStream(id,true));}
    async stopScreen(){await this.call(id=>guiStream(id,false));}
    async pressKey(key:number){
        if(key<0||key>5)return;
        // Respect the Flipper's PRESS / SHORT / RELEASE input sequence.
        await this.call(id=>guiKey(id,key,0));await pause(55);
        await this.call(id=>guiKey(id,key,2));await pause(55);
        await this.call(id=>guiKey(id,key,1));
    }
    async deviceInfo():Promise<Record<string,string>>{
        const chunks=await this.call(deviceInfoRequest);
        const result:Record<string,string>={};
        for(const chunk of chunks)if(chunk.type===RPC.DEVICE_INFO_REPLY){
            const [name,value]=deviceInfoPair(chunk.content);if(name)result[name]=value;
        }
        return result;
    }
    async list(path:string):Promise<FileItem[]>{
        if(!safePath(path))throw Error('Invalid Flipper path');
        const chunks=await this.call(id=>listRequest(id,path));
        return chunks.flatMap(c=>c.type===RPC.LIST_REPLY?parseListing(c.content):[]);
    }
    async read(path:string):Promise<Uint8Array>{
        if(!safePath(path))throw Error('Invalid Flipper path');
        const net=path==='/ext/.weave/net.request';
        const chunks=await this.call(id=>readRequest(id,path),net?240000:45000);
        const buffers=chunks.filter(c=>c.type===RPC.READ_REPLY).map(c=>parseRead(c.content));
        const size=buffers.reduce((sum,b)=>sum+b.length,0);
        if(size>(net?16*1024*1024+300:2_000_000))throw Error('File exceeds the permitted transfer limit');
        const all=new Uint8Array(size);let offset=0;
        for(const buf of buffers){all.set(buf,offset);offset+=buf.length;}
        return all;
    }
    async mkdir(path:string){if(!safePath(path))throw Error('Invalid folder name');await this.call(id=>mkdirRequest(id,path));}
    async rename(from:string,to:string){if(!safePath(from)||!safePath(to))throw Error('Invalid path');await this.call(id=>renameRequest(id,from,to));}
    async delete(path:string,recursive=true){if(!safePath(path)||path==='/ext'||path==='/int')throw Error('Invalid delete target');await this.call(id=>deleteRequest(id,path,recursive));}
    async launch(path:string){if(!/^\/ext\/apps\/(?:[\w. -]+\/)*[\w. -]+\.fap$/.test(path))throw Error('Invalid FAP path');await this.call(id=>appLaunch(id,path));}
    async write(path:string,bytes:Uint8Array,onProgress:(n:number)=>void=()=>{},pauseScreen=true){
        if(!safePath(path)||bytes.length>(path==='/ext/.weave/net.response'?16*1024*1024+100:2_000_000))throw Error('Invalid upload size');
        const before=this.uploadChain;
        let release!:()=>void;
        this.uploadChain=new Promise<void>(done=>{release=done;});
        await before;
        this.uploading=true;
        try{
            if(pauseScreen)try{await this.stopScreen();}catch{} // Streaming is optional.
            const id=this.id++;
            const result=new Promise<void>((resolve,reject)=>{
                const timer=setTimeout(()=>{this.pending.delete(id);reject(new Error('Flipper upload timed out'));},120000);
                this.pending.set(id,{resolve:()=>resolve(),reject,timer,chunks:[]});
            });
            try{
                if(bytes.length===0)await this.send(writeRequest(id,path,new Uint8Array(),false));
                for(let offset=0;offset<bytes.length;offset+=this.maxChunk){
                    const len=Math.min(this.maxChunk,bytes.length-offset);
                    await this.send(writeRequest(id,path,bytes.subarray(offset,offset+len),offset+len<bytes.length));
                    onProgress(Math.min(99,Math.round((offset+len)*100/bytes.length)));
                    if(this.maxChunk<=128)await pause(12);
                }
                await result;
                onProgress(100);
            }finally{
                if(pauseScreen)try{await this.startScreen();}catch{}
            }
        }finally{this.uploading=false;release();}
    }
    async installed():Promise<{name:string;path:string}[]>{
        const found:{name:string;path:string}[]=[];
        const queue=['/ext/apps'];let visited=0;
        while(queue.length){
            const path=queue.shift()!;
            if(++visited>100)throw Error('Too many app directories');
            for(const entry of await this.list(path)){
                const full=path+'/'+entry.name;
                if(entry.directory)queue.push(full);
                else if(entry.name.toLowerCase().endsWith('.fap'))found.push({name:entry.name,path:full});
            }
        }
        return found;
    }
}
