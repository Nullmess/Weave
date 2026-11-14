/** Official Flipper app archive + source manifest enrichment, like Android's AppCatalogClient. */
const HOST = 'https://catalog.flipperzero.one';
const CATALOG = 'https://github.com/flipperdevices/flipper-application-catalog/blob/main/';
const RAW = 'https://raw.githubusercontent.com/flipperdevices/flipper-application-catalog/main/';
const TREE = 'https://api.github.com/repos/flipperdevices/flipper-application-catalog/git/trees/main?recursive=1';
export type ManifestRow = {alias:string;name:string;category:string;description:string;icon_url:string;version:string;repository?:string;manifestUrl?:string;descriptionUrl?:string;screenshots?:string[]};
type ManifestIndexRow={alias:string;category:string;path:string};
let indexCache:{expires:number;rows:ManifestIndexRow[]}|null=null;
let expandedCache:{expires:number;rows:ManifestRow[]}|null=null;
const detailCache=new Map<string,{expires:number;data:ManifestRow}>();
const isAlias=(value:string)=>/^[\w-]{2,100}$/.test(value);
const safeRelative=(value:string)=>/^[\w. -]+(?:\/[\w. -]+)*$/.test(value)&&!value.split('/').includes('..');
const githubRepo=(value:string)=>{
    try{const u=new URL(value);return u.protocol==='https:'&&u.hostname==='github.com'&&
      /^\/[\w.-]+\/[\w.-]+\/?$/.test(u.pathname)?u.origin+u.pathname.replace(/\/$/,''):'';
    }catch{return '';}
};
const clean=(value:string)=>value.trim().replace(/^(?:["'])(.*)(?:["'])$/,'$1').trim();
const scalar=(src:string,key:string)=>{
    const m=src.match(new RegExp('^\\s*'+key+'\\s*:\\s*(.+?)\\s*$', 'm'));
    return m?clean(m[1]).replace(/\s+#.*$/,''):'';
};
async function getText(url:string,max:number,timeout=12500):Promise<string>{
    const ctrl=new AbortController(),timer=setTimeout(()=>ctrl.abort(),timeout);
    try{
        const r=await fetch(url,{signal:ctrl.signal,headers:{'User-Agent':'WeaveCompanion/1.0.0','Accept':'application/json, text/plain;q=0.9'}});
        if(!r.ok)throw Error(`Catalog HTTP ${r.status}`);
        const size=Number(r.headers.get('content-length')||0);
        if(size>max)throw Error('Catalog source too large');
        const data=await r.text();if(data.length>max)throw Error('Catalog source too large');
        return data;
    }finally{clearTimeout(timer);}
}
export function extractManifestIndex(data:unknown):ManifestIndexRow[]{
    const obj=data as {truncated?:boolean;tree?:Array<{path?:string;type?:string}>};
    if(obj.truncated)throw Error('GitHub manifest index truncated');
    const rows:ManifestIndexRow[]=[];const seen=new Set<string>();
    for(const file of obj.tree||[]){
        const match=String(file.path||'').match(/^applications\/([^/]+)\/([\w-]{2,100})\/manifest\.yml$/);
        if(match&&isAlias(match[2])&&!seen.has(match[2])){
            seen.add(match[2]);rows.push({alias:match[2],category:match[1],path:file.path!});
        }
    }
    return rows;
}
export function parseManifest(row:ManifestIndexRow,yml:string):ManifestRow{
    const category=scalar(yml,'category')||row.category;
    const name=scalar(yml,'name')||row.alias.replaceAll('_',' ');
    const version=scalar(yml,'version');
    const short=scalar(yml,'short_description');
    const description=scalar(yml,'description');
    const repo=githubRepo(scalar(yml,'origin'));
    const sha=scalar(yml,'commit_sha');
    const subdir=scalar(yml,'subdir');
    const base=repo&&/^[a-f0-9]{40}$/i.test(sha)&&(!subdir||safeRelative(subdir)) ?
      `https://raw.githubusercontent.com/${repo.slice('https://github.com/'.length)}/${sha}/${subdir?subdir.replace(/\/$/,'')+'/':''}`:'';
    const icon=scalar(yml,'icon');
    const icon_url=base&&safeRelative(icon)&&icon.toLowerCase().endsWith('.png')?base+icon:'';
    const descriptionUrl=base&&description.startsWith('@')&&safeRelative(description.slice(1))?base+description.slice(1):'';
    const screenshots:string[]=[];
    let inside=false;
    for(const line of yml.split(/\r?\n/)){
        if(/^screenshots\s*:/.test(line)){inside=true;continue;}
        if(inside&&!/^\s/.test(line)&&line.trim())break;
        const match=line.match(/^\s+-\s+['"]?([^#'"\n]+)['"]?\s*(?:#.*)?$/);
        const file=match?.[1]?.trim()||'';
        if(inside&&base&&safeRelative(file)&&/\.(png|jpe?g|webp)$/i.test(file)&&screenshots.length<12)screenshots.push(base+file);
    }
    return {alias:row.alias,name,category,description:description.startsWith('@')?short:description||short,
      icon_url,version,repository:repo||undefined,
      manifestUrl:CATALOG+row.path,descriptionUrl:descriptionUrl||undefined,screenshots};
}
async function index():Promise<ManifestIndexRow[]>{
    if(indexCache&&indexCache.expires>Date.now())return indexCache.rows;
    const tree=JSON.parse(await getText(TREE,11_000_000,22000));
    const rows=extractManifestIndex(tree);
    if(!rows.length)throw Error('No official Flipper application manifests');
    indexCache={rows,expires:Date.now()+15*60_000};
    return rows;
}
async function manifest(row:ManifestIndexRow):Promise<ManifestRow>{
    const doc=await getText(RAW+row.path,200_000);
    const item=parseManifest(row,doc);
    if(item.icon_url&&item.version)return item;
    // Android also reads a pinned application.fam when icon/version is absent
    // from manifest.yml. Never invent a logo from the catalog alias.
    const origin=githubRepo(scalar(doc,'origin'));
    const sha=scalar(doc,'commit_sha'),subdir=scalar(doc,'subdir');
    if(!origin||!/^[a-f0-9]{40}$/i.test(sha)||(subdir&&!safeRelative(subdir)))return item;
    const base=`https://raw.githubusercontent.com/${origin.slice('https://github.com/'.length)}/${sha}/${subdir?subdir.replace(/\/$/,'')+'/':''}`;
    try{
        const fam=await getText(base+'application.fam',60_000,5000);
        const icon=fam.match(/\bfap_icon\s*=\s*['"]([^'"]+)['"]/i)?.[1]||'';
        const version=fam.match(/\bfap_version\s*=\s*['"]([^'"]+)['"]/i)?.[1]||'';
        return {...item,icon_url:item.icon_url||(safeRelative(icon)&&/\.png$/i.test(icon)?base+icon:''),version:item.version||version};
    }catch{return item;}
}
async function pool<T,R>(items:T[],count:number,task:(item:T)=>Promise<R>,apply:(r:R)=>void){
    let next=0;
    await Promise.all(Array.from({length:Math.min(count,items.length)},async()=>{
        while(next<items.length){const i=next++;
            try{apply(await task(items[i]));}catch{/* One broken manifest must not hide all apps. */}
        }
    }));
}
export async function getCatalogManifests():Promise<ManifestRow[]>{
    if(expandedCache&&expandedCache.expires>Date.now())return expandedCache.rows;
    const rows=await index();
    const enriched=new Map<string,ManifestRow>();
    // Bound Github requests; all other aliases remain discoverable, never silently hidden.
    const subset=rows.filter(r=>r.alias==='totp').concat(rows.filter(r=>r.alias!=='totp').slice(0,119));
    await pool(subset,6,manifest,item=>enriched.set(item.alias,item));
    const complete=rows.map(r=>enriched.get(r.alias)||{
        alias:r.alias,name:r.alias.replaceAll('_',' '),category:r.category,
        description:'',icon_url:'',version:'',manifestUrl:CATALOG+r.path});
    expandedCache={rows:complete,expires:Date.now()+15*60_000};
    return complete;
}

export async function getCatalogVisual(alias:string):Promise<Pick<ManifestRow,'alias'|'icon_url'|'name'|'category'>>{
    if(!isAlias(alias))throw Error('Invalid application alias');
    const rows=await index(),row=rows.find(r=>r.alias===alias);
    if(!row)return {alias,icon_url:'',name:alias,category:'Other'};
    try{
        const item=await manifest(row);
        return {alias,icon_url:item.icon_url,name:item.name,category:item.category};
    }catch{return {alias,icon_url:'',name:alias,category:row.category};}
}

// Read actual Flipper .fap binary sizes, not generic repository/ZIP sizes.
export function publishedFapBytes(node:unknown,depth=0):number{
    if(depth>7||node===null||typeof node!=='object')return 0;
    if(Array.isArray(node)){
        for(const item of node.slice(0,60)){const size=publishedFapBytes(item,depth+1);if(size)return size;}
        return 0;
    }
    const record=node as Record<string,unknown>;
    const valid=(x:unknown)=>{
        const n=typeof x==='string'&&/^[0-9]{1,9}$/.test(x)?Number(x):typeof x==='number'?x:0;
        return Number.isSafeInteger(n)&&n>0&&n<=100_000_000?n:0;
    };
    for(const key of ['fap_size','fapSize','fap_size_bytes','fapSizeBytes','fap_file_size','binary_size','binarySize','build_size']){
        const n=valid(record[key]);if(n)return n;
    }
    if(/\.fap$/i.test(String(record.filename||record.file_name||record.path||''))){
        const n=valid(record.size);if(n)return n;
    }
    for(const key of ['builds','latest_build','build','artifacts','binary','binaries','releases','versions','assets','fap','application','data','result']){
        const size=publishedFapBytes(record[key],depth+1);if(size)return size;
    }
    return 0;
}
async function officialFapSize(alias:string,archiveId:string,version:string,initial:unknown):Promise<number>{
    const direct=publishedFapBytes(initial);if(direct)return direct;
    if(/^[\w.-]{1,40}$/.test(version))try{
        const url=HOST+'/api/v0/application/'+encodeURIComponent(alias)+'/versions?version='+encodeURIComponent(version);
        const versionData=JSON.parse(await getText(url,1_200_000,5500));
        const n=publishedFapBytes(versionData);if(n)return n;
    }catch{}
    if(!/^[a-f0-9]{24}$/i.test(archiveId))return 0;
    try{
        const html=await getText(HOST+'/application/'+archiveId+'/page',700_000,5500);
        const matches=html.match(/<tr\b[^>]*>[\s\S]*?<\/tr>/gi)||[];
        const filtered=matches.filter(row=>version&&row.includes(version));
        const rows=filtered.length?filtered:matches.length===1?matches:[];
        const paths=new Set<string>();
        for(const row of rows){
            for(const hit of row.matchAll(/\/api\/v0\/1\/application\/version\/[a-f0-9]{24}\/build\/f7\/[0-9]+\.[0-9]+/gi)){
                paths.add(hit[0]);if(paths.size>=3)break;
            }
            if(paths.size>=3)break;
        }
        for(const path of paths){
            const abort=new AbortController(),timer=setTimeout(()=>abort.abort(),4000);
            try{
                const response=await fetch(HOST+path,{method:'HEAD',signal:abort.signal});
                const size=Number(response.headers.get('content-length')||0);
                const type=response.headers.get('content-type')||'';
                if(response.ok&&!/text\/html|application\/json/i.test(type)&&size>0&&size<=100_000_000)return size;
            }catch{}finally{clearTimeout(timer);}
        }
    }catch{}
    return 0;
}

export async function getCatalogDetail(alias:string):Promise<ManifestRow & {_id?:string;fap_size?:number}>{
    if(!isAlias(alias))throw Error('Invalid application alias');
    const cached=detailCache.get(alias);if(cached&&cached.expires>Date.now())return cached.data;
    const rows=await index();const row=rows.find(item=>item.alias===alias);
    let source:ManifestRow|undefined;
    if(row)try{source=await manifest(row);}catch{}
    let official:any={};
    try{
        const body=JSON.parse(await getText(HOST+'/api/v0/application/'+encodeURIComponent(alias),600_000));
        if(body&&typeof body==='object'&&!Array.isArray(body)){
            const candidate=body.application||body.data||body.result||body;
            official=candidate&&typeof candidate==='object'&&!Array.isArray(candidate)?candidate:body;
        }
    }catch{}
    const id=String(official._id||official.id||'');
    const archiveId=/^[a-f0-9]{24}$/i.test(id)?id:undefined;
    let description=String(official.description||official.short_description||source?.description||'');
    let descriptionUrl=source?.descriptionUrl;
    if(source?.descriptionUrl){
        try{description=await getText(source.descriptionUrl,220_000,8000);}catch{}
    }else if(source?.repository&&description.length<200){
        // Readme text is only taken from the source recorded in the official manifest.
        const repo=source.repository.slice('https://github.com/'.length);
        for(const branch of ['main','master']){
            try{const url=`https://raw.githubusercontent.com/${repo}/${branch}/README.md`;
                description=await getText(url,200_000,6500);descriptionUrl=url;break;}catch{}
        }
    }
    const publishedVersion=String(typeof official.version==='object'?official.version?.version||'':official.version||official.latest_version?.version||source?.version||'');
    const fapSize=await officialFapSize(alias,archiveId||'',publishedVersion,official);
    const shots=Array.isArray(official.screenshots)?official.screenshots.map((v:any)=>typeof v==='string'?v:String(v?.url||v?.src||''))
      .filter((v:string)=>v.startsWith('https://')).slice(0,12):[];
    const result={alias,name:String(official.name||official.title||source?.name||alias),
      category:String(official.category||source?.category||'Other'),description,
      icon_url:String(official.icon_url||official.iconUrl||source?.icon_url||''),
      version:publishedVersion,
      repository:githubRepo(String(official.repository||''))||source?.repository,
      manifestUrl:source?.manifestUrl,descriptionUrl,
      screenshots:shots.length?shots:source?.screenshots||[],_id:archiveId,
      fap_size:fapSize};
    detailCache.set(alias,{expires:Date.now()+10*60_000,data:result});
    return result;
}
