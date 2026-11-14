/** WN2 binary mailbox carried by the authenticated Flipper Storage RPC.
 * A completed response is published by writing the `.ready` marker last.
 * Binary bodies may be up to 16 MiB; the Flipper reads in 512-byte chunks.
 */
export const NET_REQUEST = '/ext/.weave/net.request';
export const NET_RESPONSE = '/ext/.weave/net.response';
export const NET_READY = '/ext/.weave/net.response.ready';
export const NET_LINK = '/ext/.weave/net.link';
export const NET_BODY_LIMIT = 16 * 1024 * 1024;
const ascii = new TextEncoder();
const asciiDecoder = new TextDecoder('ascii',{fatal:true});
const VERBS = new Set(['GET','POST','PUT','PATCH','DELETE','HEAD',
  'TCP_OPEN','TCP_SEND','TCP_RECV','TCP_CLOSE','UDP_OPEN','UDP_SEND','UDP_RECV','UDP_CLOSE',
  'WS_OPEN','WS_SEND','WS_RECV','WS_CLOSE']);
export type NetworkRequest={id:string;verb:string;target:string;data:Uint8Array};
export type NetworkResult={status:number;code:number;data:Uint8Array};

export function parseNetworkRequest(bytes:Uint8Array):NetworkRequest|null {
    if(!(bytes instanceof Uint8Array) || bytes.length<14 || bytes.length>NET_BODY_LIMIT+300)return null;
    const end=bytes.indexOf(10);
    if(end<0||end>280)return null;
    let header:string;
    try{header=asciiDecoder.decode(bytes.subarray(0,end));}catch{return null;}
    const match=/^WN2\|([1-9][0-9]{0,9})\|([A-Z_]{3,10})\|([^|\r\n\x00-\x1f]{1,191})\|([0-9]{1,8})$/.exec(header);
    if(!match||Number(match[1])>0xffffffff||!VERBS.has(match[2]))return null;
    const length=Number(match[4]);
    if(length>NET_BODY_LIMIT || end+1+length!==bytes.length)return null;
    if(['GET','HEAD'].includes(match[2])&&length>0)return null;
    if(!match[2].endsWith('SEND') && !['POST','PUT','PATCH','DELETE'].includes(match[2]) && length>0)return null;
    return {id:match[1],verb:match[2],target:match[3],data:bytes.subarray(end+1)};
}
export function networkReply(id:string,result:NetworkResult):Uint8Array {
    if(!/^[1-9][0-9]{0,9}$/.test(id)||Number(id)>0xffffffff||
       !Number.isInteger(result.status)||result.status<0||result.status>7||
       !Number.isInteger(result.code)||result.code<0||result.code>65535||
       !(result.data instanceof Uint8Array)||result.data.length>NET_BODY_LIMIT)
       throw Error('Invalid WN2 response');
    const header=ascii.encode(`WN2|${id}|${result.status}|${result.code}|${result.data.length}\n`);
    const output=new Uint8Array(header.length+result.data.length);
    output.set(header);output.set(result.data,header.length);
    return output;
}
export function networkReady(id:string):Uint8Array {
    if(!/^[1-9][0-9]{0,9}$/.test(id)||Number(id)>0xffffffff)throw Error('Invalid WN2 ID');
    return ascii.encode(`WN2|${id}\n`);
}
export function networkHeartbeat(transport:'USB'|'BLE',sequence:number):Uint8Array {
    if(!Number.isInteger(sequence)||sequence<0||sequence>0xffffffff)throw Error('Invalid heartbeat sequence');
    return ascii.encode(`WN1|${transport}|${sequence}\n`);
}
