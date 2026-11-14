/** ZIP STORE writer. No external dependency; preserves paths and binary bytes. */
export type ArchiveItem={path:string;data:Uint8Array;directory:boolean};
function write16(out:number[],n:number){out.push(n&255,(n>>>8)&255);}
function write32(out:number[],n:number){out.push(n&255,(n>>>8)&255,(n>>>16)&255,(n>>>24)&255);}
let table:Uint32Array|undefined;
function crc32(data:Uint8Array):number{
  if(!table){table=new Uint32Array(256);for(let i=0;i<256;i++){
    let c=i;for(let j=0;j<8;j++)c=(c&1)?0xedb88320^(c>>>1):c>>>1;table[i]=c>>>0;
  }}
  let c=0xffffffff;for(const b of data)c=table[(c^b)&255]^(c>>>8);return (c^0xffffffff)>>>0;
}
export function createZip(entries:ArchiveItem[]):Uint8Array{
  const encoder=new TextEncoder(),out:number[]=[],central:number[]=[];
  if(entries.length>4096)throw Error('More than 4096 archive entries');
  for(const entry of entries){
    let name=entry.path.replaceAll('\\','/');
    if(!name||name.startsWith('/')||name.split('/').some(x=>!x||x==='.'||x==='..'))throw Error('Invalid ZIP path');
    if(entry.directory&&!name.endsWith('/'))name+='/';
    const nameBytes=encoder.encode(name),body=entry.directory?new Uint8Array():entry.data;
    if(nameBytes.length>65535||body.length>2_000_000||out.length+body.length>32_000_000)throw Error('ZIP export limit exceeded');
    const offset=out.length,crc=crc32(body);
    write32(out,0x04034b50);write16(out,20);write16(out,0x0800);write16(out,0);write16(out,0);write16(out,0);
    write32(out,crc);write32(out,body.length);write32(out,body.length);write16(out,nameBytes.length);write16(out,0);
    out.push(...nameBytes);for(let i=0;i<body.length;i+=8192)out.push(...body.subarray(i,i+8192));
    write32(central,0x02014b50);write16(central,20);write16(central,20);write16(central,0x0800);write16(central,0);
    write16(central,0);write16(central,0);write32(central,crc);write32(central,body.length);write32(central,body.length);
    write16(central,nameBytes.length);write16(central,0);write16(central,0);write16(central,0);write16(central,0);
    write32(central,entry.directory?0x10:0);write32(central,offset);central.push(...nameBytes);
  }
  const centralOffset=out.length;out.push(...central);
  write32(out,0x06054b50);write16(out,0);write16(out,0);write16(out,entries.length);write16(out,entries.length);
  write32(out,central.length);write32(out,centralOffset);write16(out,0);
  return new Uint8Array(out);
}
