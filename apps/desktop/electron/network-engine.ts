/* WN2 transport backend, deliberately separate from Electron so it can be
 * unit-tested. The user enables the bridge; developer-selected endpoints are
 * validated for scheme/format. No UI host list or automatic HTTP redirects.
 */
import net from 'node:net';
import dgram from 'node:dgram';

export const NET_MAX_BYTES=16*1024*1024;
const MAX_SESSIONS=4;
const QUEUE_LIMIT=1024*1024;
const SESSION_IDLE_MS=5*60*1000;
export type NetworkOperation={id:string;verb:string;target:string;data:Uint8Array};
export type NetworkResult={status:number;code:number;data:Uint8Array};
const respond=(status=0,code=0,data:Uint8Array=new Uint8Array()):NetworkResult=>({status,code,data});
const ASCII_HOST=/^(?=.{1,253}$)[a-z0-9](?:[a-z0-9.-]*[a-z0-9])?$/i;
const validHost=(host:string)=>net.isIP(host.replace(/^\[|\]$/g,''))!==0 || (ASCII_HOST.test(host)&&!host.includes('..'));
type Session={kind:'TCP'|'UDP'|'WS';socket:net.Socket|dgram.Socket|WebSocket;queue:Buffer[];
    queued:number;dead:boolean;last:number;watch:boolean};
export class NetworkEngine {
    private sessions=new Map<number,Session>();
    private serial=0;
    private safeUrl(target:string,kind:'http'|'ws'):URL {
        const u=new URL(target);
        if(!validHost(u.hostname)||u.username||u.password||u.hash ||
            !(kind==='http' ? ['http:','https:'].includes(u.protocol) : ['ws:','wss:'].includes(u.protocol)))
            throw Error('Invalid network destination');
        return u;
    }
    private endpoint(target:string):{host:string;port:number}{
        /* Brackets are mandatory for IPv6 so that the port is unambiguous. */
        const match=/^(?:\[([^\]\s|%]+)\]|([^:/\s|]+)):([0-9]{1,5})$/.exec(target);
        if(!match)throw Error('Invalid endpoint');
        const host=match[1]??match[2];
        if(match[1] ? net.isIP(host)!==6 : !validHost(host))
            throw Error('Invalid endpoint');
        const port=Number(match[3]);
        if(port<1||port>65535)throw Error('Invalid port');
        return {host,port};
    }
    private close(id:number){
        const s=this.sessions.get(id);if(!s)return;
        this.sessions.delete(id);s.dead=true;
        try {
            if(s.kind==='TCP')(s.socket as net.Socket).destroy();
            else if(s.kind==='UDP')(s.socket as dgram.Socket).close();
            else (s.socket as WebSocket).close();
        }catch{}
    }
    reset(){for(const id of [...this.sessions.keys()])this.close(id);}
    private slot():number{
        for(const [id,s] of this.sessions)if(Date.now()-s.last>SESSION_IDLE_MS||s.dead)this.close(id);
        if(this.sessions.size>=MAX_SESSIONS)throw Error('Session limit');
        for(let i=0;i<65535;i++){
            this.serial=this.serial%65535+1;
            if(!this.sessions.has(this.serial))return this.serial;
        }
        throw Error('No available session');
    }
    private enqueue(s:Session,chunk:Buffer){
        if(s.dead)return;
        if(s.queued+chunk.length>QUEUE_LIMIT){s.dead=true;return;}
        if(chunk.length){s.queue.push(chunk);s.queued+=chunk.length;}
    }
    private dequeue(s:Session,limit=65536):Uint8Array{
        s.last=Date.now();
        if(s.dead){throw Error('Closed or overflowed session');}
        let used=0;const chunks:Buffer[]=[];
        while(s.queue.length&&used<limit){
            const buffer=s.queue[0];const take=Math.min(buffer.length,limit-used);
            chunks.push(buffer.subarray(0,take));
            used+=take;s.queued-=take;
            if(take===buffer.length)s.queue.shift();else s.queue[0]=buffer.subarray(take);
        }
        return Buffer.concat(chunks,used);
    }
    /* Best-effort unsolicited socket events. The renderer publishes one event
     * at a time through the Storage RPC mailbox and waits for its ack marker
     * to disappear before draining another buffered message. */
    nextEvent():{kind:'TCP'|'UDP'|'WS';session:number;data:Uint8Array}|null{
        for(const [session,s] of this.sessions){
            if(s.watch&&!s.dead&&s.queued>0)
                return {kind:s.kind,session,data:this.dequeue(s,4096)};
        }
        return null;
    }
    private getSession(target:string,kind:string):{id:number;s:Session}{
        if(!/^[1-9][0-9]{0,4}$/.test(target))throw Error('Bad session ID');
        const id=Number(target),s=this.sessions.get(id);
        if(!s||s.kind!==kind||Date.now()-s.last>SESSION_IDLE_MS){if(s)this.close(id);throw Error('Unknown session');}
        s.last=Date.now();return {id,s};
    }
    private async openSocket(kind:'TCP'|'UDP'|'WS',target:string):Promise<NetworkResult>{
        const id=this.slot();
        const session:Session={kind,socket:null as any,queue:[],queued:0,dead:false,last:Date.now(),watch:false};
        if(kind==='TCP'){
            const {host,port}=this.endpoint(target);
            const socket=net.createConnection({host,port});session.socket=socket;
            socket.on('data',(d:Buffer|string)=>this.enqueue(session,Buffer.isBuffer(d)?d:Buffer.from(d)));
            socket.on('error',()=>{session.dead=true;});
            socket.on('close',()=>{session.dead=true;});
            await new Promise<void>((resolve,reject)=>{
                const timer=setTimeout(()=>{socket.destroy();reject(Error('TCP connect timeout'));},8000);
                socket.once('connect',()=>{clearTimeout(timer);resolve();});
                socket.once('error',e=>{clearTimeout(timer);reject(e);});
            });
        }else if(kind==='UDP'){
            const {host,port}=this.endpoint(target);
            const socket=dgram.createSocket(net.isIP(host)===6?'udp6':'udp4');session.socket=socket;
            socket.on('message',packet=>this.enqueue(session,packet));
            socket.on('error',()=>{session.dead=true;});
            await new Promise<void>((resolve,reject)=>{
                const timer=setTimeout(()=>{socket.close();reject(Error('UDP timeout'));},8000);
                socket.connect(port,host,()=>{clearTimeout(timer);resolve();});
                socket.once('error',e=>{clearTimeout(timer);reject(e);});
            });
        }else{
            const url=this.safeUrl(target,'ws');
            const socket=new WebSocket(url.toString());session.socket=socket;
            socket.binaryType='arraybuffer';
            socket.addEventListener('message',event=>{
                if(typeof event.data==='string')this.enqueue(session,Buffer.from(event.data));
                else if(event.data instanceof ArrayBuffer)this.enqueue(session,Buffer.from(event.data));
                else session.dead=true;
            });
            socket.addEventListener('error',()=>{session.dead=true;});
            socket.addEventListener('close',()=>{session.dead=true;});
            await new Promise<void>((resolve,reject)=>{
                const timer=setTimeout(()=>{socket.close();reject(Error('WebSocket connect timeout'));},8000);
                socket.addEventListener('open',()=>{clearTimeout(timer);resolve();},{once:true});
                socket.addEventListener('error',()=>{clearTimeout(timer);reject(Error('WebSocket handshake failed'));},{once:true});
            });
        }
        this.sessions.set(id,session);
        return respond(0,id);
    }
    private async socketOperation(verb:string,target:string,data:Uint8Array):Promise<NetworkResult>{
        const [kind,action]=verb.split('_');
        if(!['TCP','UDP','WS'].includes(kind)||!['OPEN','SEND','RECV','CLOSE','WATCH'].includes(action))
            return respond(3);
        if(action==='OPEN')return this.openSocket(kind as 'TCP'|'UDP'|'WS',target);
        const {id,s}=this.getSession(target,kind);
        if(action==='CLOSE'){this.close(id);return respond();}
        if(action==='WATCH'){
            if(data.length!==1||data[0]>1)return respond(3);
            s.watch=!!data[0];return respond();
        }
        if(action==='RECV')return respond(0,0,this.dequeue(s));
        if(data.length>NET_MAX_BYTES) return respond(4);
        if(kind==='TCP') await new Promise<void>((resolve,reject)=>
            (s.socket as net.Socket).write(data,e=>e?reject(e):resolve()));
        if(kind==='UDP') await new Promise<void>((resolve,reject)=>
            (s.socket as dgram.Socket).send(data,e=>e?reject(e):resolve()));
        if(kind==='WS') {
            if((s.socket as WebSocket).readyState!==WebSocket.OPEN)throw Error('Socket closed');
            (s.socket as WebSocket).send(data);
        }
        return respond();
    }
    /* HTTPX: HX1, method length, method, header count, repeated
     * (name length, value length LE16, name, value), entity bytes. */
    private decodeHttpX(payload:Uint8Array):{method:string;headers:Record<string,string>;body:Uint8Array}{
        const decoder=new TextDecoder('ascii',{fatal:true});
        if(payload.length<6||String.fromCharCode(...payload.subarray(0,3))!=='HX1')
            throw Error('Invalid HTTPX envelope');
        let at=3;
        const methodLength=payload[at++];
        if(methodLength<3||methodLength>7||at+methodLength+1>payload.length)throw Error('Invalid HTTPX method');
        const method=decoder.decode(payload.subarray(at,at+methodLength));at+=methodLength;
        if(!['GET','POST','PUT','PATCH','DELETE','HEAD'].includes(method))throw Error('Invalid HTTPX method');
        const count=payload[at++];
        if(count>8)throw Error('Too many HTTPX headers');
        const headers:Record<string,string>={};
        const forbidden=new Set(['host','content-length','connection','transfer-encoding','upgrade','keep-alive','te','trailer','proxy-authorization','proxy-connection']);
        for(let i=0;i<count;i++){
            if(at+3>payload.length)throw Error('Invalid HTTPX header');
            const nameLen=payload[at++],valueLen=payload[at++]|(payload[at++]<<8);
            if(!nameLen||nameLen>64||valueLen>256||at+nameLen+valueLen>payload.length)
                throw Error('Invalid HTTPX header');
            const name=decoder.decode(payload.subarray(at,at+nameLen));at+=nameLen;
            const value=decoder.decode(payload.subarray(at,at+valueLen));at+=valueLen;
            if(!/^[a-z0-9-]+$/i.test(name)||!/^[ -~]*$/.test(value)||forbidden.has(name.toLowerCase())||
                Object.keys(headers).some(key=>key.toLowerCase()===name.toLowerCase()))
                throw Error('Invalid HTTPX header');
            headers[name]=value;
        }
        const body=payload.subarray(at);
        if(['GET','HEAD'].includes(method)&&body.length)throw Error('Invalid HTTPX body');
        return {method,headers,body};
    }
    private async http(method:string,target:string,body:Uint8Array,extra:Record<string,string>={}):Promise<NetworkResult>{
        const u=this.safeUrl(target,'http');
        if(['GET','HEAD'].includes(method)&&body.length) return respond(3);
        const ctrl=new AbortController();
        const timer=setTimeout(()=>ctrl.abort(),60000);
        try {
            // DOM BodyInit requires an ArrayBuffer-backed view (not SharedArrayBuffer).
            // Copy incoming RPC bytes into that explicitly typed, fetch-safe buffer.
            const requestBody=body.length?new Uint8Array(new ArrayBuffer(body.byteLength)):undefined;
            if(requestBody)requestBody.set(body);
            const headers=new Headers({'User-Agent':'WeaveCompanion/3'});
            if(body.length)headers.set('Content-Type','application/octet-stream');
            for(const [name,value] of Object.entries(extra))headers.set(name,value);
            const res=await fetch(u,{method,redirect:'error',signal:ctrl.signal,
                body:requestBody,headers});
            if(Number(res.headers.get('content-length')||0)>NET_MAX_BYTES)return respond(4);
            const chunks:Buffer[]=[];let n=0;
            if(res.body){
                const reader=res.body.getReader();
                try {for(;;){
                    const part=await reader.read();if(part.done)break;
                    n+=part.value.byteLength;
                    if(n>NET_MAX_BYTES){await reader.cancel();return respond(4);}
                    chunks.push(Buffer.from(part.value));
                }}finally{reader.releaseLock();}
            }
            return respond(0,res.status,Buffer.concat(chunks,n));
        }finally{clearTimeout(timer);}
    }
    async execute(op:NetworkOperation,approved:boolean):Promise<NetworkResult>{
        if(!approved)return respond(7);
        if(!/^[1-9][0-9]{0,9}$/.test(op.id)||Number(op.id)>0xffffffff||
            typeof op.verb!=='string'||typeof op.target!=='string'||
            op.target.length>383||!(op.data instanceof Uint8Array)||op.data.length>NET_MAX_BYTES)
            return respond(3);
        try {
            if(op.verb==='HTTPX') {
                if(!op.target.startsWith('https://'))return respond(7);
                const x=this.decodeHttpX(op.data);
                return await this.http(x.method,op.target,x.body,x.headers);
            }
            if(['GET','POST','PUT','PATCH','DELETE','HEAD'].includes(op.verb))
                return await this.http(op.verb,op.target,op.data);
            return await this.socketOperation(op.verb,op.target,op.data);
        } catch(e){
            const detail=String(e);
            if(/not allowed|Invalid URL|Invalid network destination|Invalid endpoint|Bad session|Unknown session|Forbidden|Invalid port/i.test(detail))return respond(7);
            if(/Session limit|overflow|MAX/i.test(detail))return respond(4);
            if(/timeout|aborted|AbortError/i.test(detail))return respond(5);
            return respond(6);
        }
    }
}
