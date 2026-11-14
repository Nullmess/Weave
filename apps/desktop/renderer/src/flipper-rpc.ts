/** Flipper Zero's length-prefixed PB.Main RPC, shared wire format with Android's FlipperRpc. */
export const RPC = {
  LIST: 7, LIST_REPLY: 8, READ: 9, READ_REPLY: 10, WRITE: 11,
  DELETE: 12, MKDIR: 13, APP_START: 16, GUI_START: 20, GUI_STOP: 21,
  GUI_FRAME: 22, GUI_INPUT: 23, RENAME: 30, DEVICE_INFO: 32, DEVICE_INFO_REPLY: 33,
} as const;
const utf8 = new TextEncoder();
const decodeText = new TextDecoder();
function varint(value:number):number[]{
  if(!Number.isSafeInteger(value)||value<0)throw Error('Invalid protobuf integer');
  const result:number[]=[];
  do{const b=value%128;value=Math.floor(value/128);result.push(b|(value?128:0));}while(value);
  return result;
}
export const bytesField=(field:number,data:Uint8Array):number[]=>[...varint(field*8+2),...varint(data.length),...data];
export const intField=(field:number,value:number):number[]=>[...varint(field*8),...varint(value)];
export const textField=(field:number,value:string):number[]=>bytesField(field,utf8.encode(value));
export function message(id:number,field:number,body:number[]=[],hasNext=false):Uint8Array{
  const data=new Uint8Array([...intField(1,id),...(hasNext?intField(3,1):[]),...bytesField(field,new Uint8Array(body))]);
  return new Uint8Array([...varint(data.length),...data]);
}
export const listRequest=(id:number,path:string)=>message(id,RPC.LIST,textField(1,path));
export const readRequest=(id:number,path:string)=>message(id,RPC.READ,textField(1,path));
export const mkdirRequest=(id:number,path:string)=>message(id,RPC.MKDIR,textField(1,path));
export const deleteRequest=(id:number,path:string,recursive=false)=>message(id,RPC.DELETE,[...textField(1,path),...(recursive?intField(2,1):[])]);
export const renameRequest=(id:number,from:string,to:string)=>message(id,RPC.RENAME,[...textField(1,from),...textField(2,to)]);
export const guiStream=(id:number,on:boolean)=>message(id,on?RPC.GUI_START:RPC.GUI_STOP);
export const guiKey=(id:number,key:number,type:number)=>message(id,RPC.GUI_INPUT,[...intField(1,key),...intField(2,type)]);
export const deviceInfoRequest=(id:number)=>message(id,RPC.DEVICE_INFO);
export const appLaunch=(id:number,path:string)=>message(id,RPC.APP_START,textField(1,path));
export function writeRequest(id:number,path:string,data:Uint8Array,more=false){return message(id,RPC.WRITE,[...textField(1,path),...bytesField(2,new Uint8Array(bytesField(4,data)))],more);}
export type Field={field:number;wire:number;value:number|Uint8Array};
export function fields(bytes:Uint8Array):Field[]{
  const output:Field[]=[];let pos=0;
  const readVar=()=>{let result=0;let multiplier=1;for(let j=0;j<10;j++){
    if(pos>=bytes.length)throw Error('Truncated varint');
    const b=bytes[pos++];result+=(b&127)*multiplier;
    if(b<128){if(!Number.isSafeInteger(result))throw Error('Unsafe varint');return result;}
    multiplier*=128;
  }throw Error('Oversized varint');};
  while(pos<bytes.length){const tag=readVar();const field=Math.floor(tag/8),wire=tag&7;
    if(!field)throw Error('Invalid field');
    if(wire===0){output.push({field,wire,value:readVar()});}
    else if(wire===2){const len=readVar();if(len<0||len>bytes.length-pos)throw Error('Truncated field');output.push({field,wire,value:bytes.slice(pos,pos+len)});pos+=len;}
    else if(wire===1||wire===5){const len=wire===1?8:4;if(len>bytes.length-pos)throw Error('Truncated fixed field');pos+=len;}
    else throw Error('Unsupported protobuf wire type');
  }
  return output;
}
export type Reply={id:number;status:number;type:number;hasNext:boolean;content:Uint8Array};
export function parseReply(bytes:Uint8Array):Reply{
  const result:Reply={id:-1,status:0,type:0,hasNext:false,content:new Uint8Array()};
  for(const f of fields(bytes)){
    if(f.field===1&&f.wire===0)result.id=f.value as number;
    else if(f.field===2&&f.wire===0)result.status=f.value as number;
    else if(f.field===3&&f.wire===0)result.hasNext=!!f.value;
    else if(f.field>3&&f.wire===2){result.type=f.field;result.content=f.value as Uint8Array;}
  }
  return result;
}
export type FileItem={name:string;directory:boolean;size:number};
export function parseListing(body:Uint8Array):FileItem[]{
  const list:FileItem[]=[];
  for(const f of fields(body)){
    if(f.field!==1||f.wire!==2)continue;
    let name='',directory=false,size=0;
    for(const child of fields(f.value as Uint8Array)){
      if(child.field===1&&child.wire===0)directory=(child.value as number)===1;
      if(child.field===2&&child.wire===2)name=decodeText.decode(child.value as Uint8Array);
      if(child.field===3&&child.wire===0)size=child.value as number;
    }
    if(name&&name!=='.'&&name!=='..')list.push({name,directory,size});
  }
  return list;
}
export function parseRead(body:Uint8Array):Uint8Array{
  for(const f of fields(body))if(f.field===1&&f.wire===2){
    for(const child of fields(f.value as Uint8Array))if(child.field===4&&child.wire===2)return child.value as Uint8Array;
  }
  return new Uint8Array();
}
export function parseScreen(body:Uint8Array):Uint8Array{
  for(const f of fields(body))if(f.field===1&&f.wire===2)return f.value as Uint8Array;
  return new Uint8Array();
}
export function deviceInfoPair(body:Uint8Array):[string,string]{
  let name='',value='';for(const f of fields(body)){
    if(f.field===1&&f.wire===2)name=decodeText.decode(f.value as Uint8Array);
    if(f.field===2&&f.wire===2)value=decodeText.decode(f.value as Uint8Array);
  }return [name,value];
}
export class Framer {
  private buffer=new Uint8Array();
  clear(){this.buffer=new Uint8Array();}
  push(data:Uint8Array):Reply[]{
    const updated=new Uint8Array(this.buffer.length+data.length);
    updated.set(this.buffer);updated.set(data,this.buffer.length);
    this.buffer=updated;
    if(this.buffer.length>1_500_000){this.clear();throw Error('RPC input exceeds buffer limit');}
    const replies:Reply[]=[];
    while(this.buffer.length){
      let length=0,multiplier=1,prefix=0,done=false;
      for(let i=0;i<Math.min(5,this.buffer.length);i++){
        const b=this.buffer[i];length+=(b&127)*multiplier;prefix++;
        if(b<128){done=true;break;}multiplier*=128;
      }
      if(!done){if(prefix>=5)throw Error('Invalid RPC length');break;}
      if(length>1_048_576)throw Error('RPC frame too large');
      if(this.buffer.length<prefix+length)break;
      replies.push(parseReply(this.buffer.slice(prefix,prefix+length)));
      this.buffer=this.buffer.slice(prefix+length);
    }
    return replies;
  }
}
export function screenPixels(pages:Uint8Array):Uint8ClampedArray{
  if(pages.length!==1024)throw Error('Invalid 128x64 framebuffer');
  const rgba=new Uint8ClampedArray(128*64*4);
  for(let y=0;y<64;y++)for(let x=0;x<128;x++){
    const dark=(pages[(y>>3)*128+x]&(1<<(y&7)))!==0;
    const offset=(y*128+x)*4;
    rgba[offset]=dark?48:255;rgba[offset+1]=dark?33:163;rgba[offset+2]=dark?10:35;rgba[offset+3]=255;
  }
  return rgba;
}
