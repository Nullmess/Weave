package io.github.nullmess.weave;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.SecureRandom;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Base64;
import java.util.HashMap;
import java.util.Map;
import java.util.Locale;
import javax.net.ssl.SSLParameters;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.SSLSocketFactory;

/** Android network backend for WN2 (developer-selected endpoints).
 * Uses verified TLS, real PATCH and RFC6455 masked client frames without
 * adding non-platform dependencies. Runs off the Android main thread. */
final class NetworkGateway {
    static final class Result {
        final int status,code;
        final byte[] data;
        Result(int status,int code,byte[] data){this.status=status;this.code=code;this.data=data;}
    }
    private static Result result(int status,int code,byte[] data){return new Result(status,code,data);}
    private static Result ok(){return result(0,0,new byte[0]);}
    private static final int LIMIT=NetworkMailbox.MAX_BODY, MAX_SESSIONS=4;
    private static final long SESSION_TTL=300000L;
    private static final Map<Integer,Session> sessions=new HashMap<>();
    private static final SecureRandom random=new SecureRandom();
    private static int nextSession=1;
    private static final class Session {
        final String kind;
        final Socket tcp;
        final DatagramSocket udp;
        final InputStream in;
        final OutputStream out;
        long last=System.currentTimeMillis();
        boolean watch;
        byte[] eventPending;
        int eventOffset;
        Session(String kind,Socket tcp,DatagramSocket udp) throws IOException {
            this.kind=kind;this.tcp=tcp;this.udp=udp;
            this.in=tcp==null?null:tcp.getInputStream();
            this.out=tcp==null?null:tcp.getOutputStream();
        }
        void close(){try{if(tcp!=null)tcp.close();}catch(IOException ignored){}
            if(udp!=null)udp.close();}
    }
    static synchronized void reset(){for(Session s:sessions.values())s.close();sessions.clear();}
    private static void permitted(String host){
        if(host==null || !host.matches("(?i)(?=.{1,253}$)[a-z0-9](?:[a-z0-9.-]*[a-z0-9])?") ||
            host.contains("..")) throw new IllegalArgumentException("Invalid destination");
    }
    private static URL url(String target,String kind) throws IOException {
        String normalized=target;
        if(kind.equals("WS")){
            if(target.startsWith("wss://"))normalized="https://"+target.substring(6);
            else if(target.startsWith("ws://"))normalized="http://"+target.substring(5);
            else throw new IllegalArgumentException("Unsupported WS scheme");
        }
        URL u=new URL(normalized);
        if(u.getUserInfo()!=null||u.getRef()!=null)throw new IllegalArgumentException("User info/fragment not permitted");
        String protocol=u.getProtocol();
        if(!protocol.equals("http")&&!protocol.equals("https"))
            throw new IllegalArgumentException("Unsupported scheme");
        permitted(u.getHost());
        return u;
    }
    private static Socket connect(String host,int port,boolean tls) throws IOException {
        Socket plain=new Socket();plain.connect(new InetSocketAddress(host,port),8000);
        plain.setSoTimeout(30000);
        if(!tls)return plain;
        try {
            SSLSocket ssl=(SSLSocket)((SSLSocketFactory)SSLSocketFactory.getDefault()).createSocket(plain,host,port,true);
            SSLParameters parameters=ssl.getSSLParameters();
            parameters.setEndpointIdentificationAlgorithm("HTTPS");
            ssl.setSSLParameters(parameters);
            ssl.startHandshake();
            ssl.setSoTimeout(30000);
            return ssl;
        }catch(IOException error){plain.close();throw error;}
    }
    private static String readLine(InputStream in) throws IOException {
        ByteArrayOutputStream buffer=new ByteArrayOutputStream();
        int c;
        while((c=in.read())!=-1){
            if(c=='\n')break;
            if(c!='\r')buffer.write(c);
            if(buffer.size()>8192)throw new IOException("Header too large");
        }
        if(c==-1)throw new IOException("Truncated line");
        return buffer.toString("ISO-8859-1");
    }
    private static void append(ByteArrayOutputStream out,byte[] bytes,int count) throws IOException {
        if(count>LIMIT-out.size())throw new TooLarge();
        out.write(bytes,0,count);
    }
    private static final class TooLarge extends IOException {}
    private static final class HttpX {
        final String method;
        final Map<String,String> headers;
        final byte[] body;
        HttpX(String m,Map<String,String> h,byte[] b){method=m;headers=h;body=b;}
    }
    private static HttpX decodeHttpX(byte[] payload) {
        if(payload.length<6||payload[0]!='H'||payload[1]!='X'||payload[2]!='1')
            throw new IllegalArgumentException("HTTPX magic");
        int at=3;
        int mlen=payload[at++]&255;
        if(mlen<3||mlen>7||at+mlen+1>payload.length)
            throw new IllegalArgumentException("HTTPX method");
        String method=new String(payload,at,mlen,StandardCharsets.US_ASCII);at+=mlen;
        if(!Arrays.asList("GET","POST","PUT","PATCH","DELETE","HEAD").contains(method))
            throw new IllegalArgumentException("HTTPX method");
        int count=payload[at++]&255;
        if(count>8)throw new IllegalArgumentException("HTTPX headers");
        Map<String,String> headers=new HashMap<>();
        for(int i=0;i<count;i++){
            if(at+3>payload.length)throw new IllegalArgumentException("HTTPX header");
            int nl=payload[at++]&255,vl=(payload[at++]&255)|((payload[at++]&255)<<8);
            if(nl<1||nl>64||vl>256||at+nl+vl>payload.length)
                throw new IllegalArgumentException("HTTPX header");
            for(int j=at;j<at+nl+vl;j++)if((payload[j]&255)<32||(payload[j]&255)>126)
                throw new IllegalArgumentException("HTTPX ASCII");
            String name=new String(payload,at,nl,StandardCharsets.US_ASCII);at+=nl;
            String value=new String(payload,at,vl,StandardCharsets.US_ASCII);at+=vl;
            String lower=name.toLowerCase(Locale.ROOT);
            if(!name.matches("[a-zA-Z0-9-]+")||
               Arrays.asList("host","content-length","connection","transfer-encoding","upgrade",
                   "keep-alive","te","trailer","proxy-authorization","proxy-connection").contains(lower))
                throw new IllegalArgumentException("HTTPX forbidden header");
            for(String existing:headers.keySet())
                if(existing.equalsIgnoreCase(name))throw new IllegalArgumentException("Duplicate header");
            headers.put(name,value);
        }
        byte[] body=Arrays.copyOfRange(payload,at,payload.length);
        if((method.equals("GET")||method.equals("HEAD"))&&body.length>0)
            throw new IllegalArgumentException("HTTPX unexpected body");
        return new HttpX(method,headers,body);
    }
    private static Result http(String method,String target,byte[] data) throws IOException {
        return http(method,target,data,new HashMap<>());
    }
    private static Result http(String method,String target,byte[] data,Map<String,String> headers) throws IOException {
        URL u=url(target,"HTTP");
        if((method.equals("GET")||method.equals("HEAD"))&&data.length!=0)return result(3,0,new byte[0]);
        String host=u.getHost();
        boolean tls=u.getProtocol().equals("https");
        int port=u.getPort()==-1?(tls?443:80):u.getPort();
        try(Socket socket=connect(host,port,tls)){
            String resource=u.getFile();if(resource.isEmpty())resource="/";
            if(resource.indexOf('\r')>=0||resource.indexOf('\n')>=0)throw new IllegalArgumentException("Bad path");
            String authority=host+((tls&&port==443)||(!tls&&port==80)?"":":"+port);
            StringBuilder head=new StringBuilder(method+" "+resource+" HTTP/1.1\r\nHost: "+authority+
                "\r\nUser-Agent: WeaveCompanion/3\r\nConnection: close\r\nAccept-Encoding: identity\r\n");
            boolean contentType=false;
            for(Map.Entry<String,String> header:headers.entrySet()) {
                if(header.getKey().equalsIgnoreCase("content-type"))contentType=true;
                head.append(header.getKey()).append(": ").append(header.getValue()).append("\r\n");
            }
            if(data.length>0&&!contentType)head.append("Content-Type: application/octet-stream\r\n");
            head.append("Content-Length: ").append(data.length).append("\r\n\r\n");
            OutputStream out=socket.getOutputStream();
            out.write(head.toString().getBytes(StandardCharsets.US_ASCII));out.write(data);out.flush();
            InputStream in=socket.getInputStream();
            String status=readLine(in);
            if(!status.matches("HTTP/1\\.[01] [1-5][0-9][0-9](?: .*)?"))throw new IOException("Bad HTTP response");
            int code=Integer.parseInt(status.substring(9,12));
            int contentLength=-1;boolean chunked=false;
            for(int lines=0;lines<160;lines++){
                String line=readLine(in);if(line.isEmpty())break;
                int colon=line.indexOf(':');if(colon<0)throw new IOException("Malformed header");
                String key=line.substring(0,colon).trim().toLowerCase(Locale.ROOT);
                String val=line.substring(colon+1).trim().toLowerCase(Locale.ROOT);
                if(key.equals("content-length")){
                    try{contentLength=Integer.parseInt(val);}catch(NumberFormatException e){throw new IOException("Content length");}
                }
                if(key.equals("transfer-encoding")&&val.contains("chunked"))chunked=true;
                if(lines==159)throw new IOException("Too many headers");
            }
            if(contentLength>LIMIT)throw new TooLarge();
            if(method.equals("HEAD")||code==204||code==304||code<200)return result(0,code,new byte[0]);
            ByteArrayOutputStream body=new ByteArrayOutputStream();
            byte[] buffer=new byte[4096];
            if(chunked){
                while(true){
                    String sizeLine=readLine(in).split(";",2)[0].trim();
                    int size;
                    try{size=Integer.parseInt(sizeLine,16);}catch(NumberFormatException e){throw new IOException("Chunk size");}
                    if(size<0||size>LIMIT-body.size())throw new TooLarge();
                    if(size==0){for(int i=0;i<64;i++)if(readLine(in).isEmpty())break;break;}
                    int left=size;
                    while(left>0){int n=in.read(buffer,0,Math.min(buffer.length,left));if(n<0)throw new IOException("Truncated chunk");append(body,buffer,n);left-=n;}
                    if(in.read()!='\r'||in.read()!='\n')throw new IOException("Chunk delimiter");
                }
            }else{
                int left=contentLength;
                while(left!=0){int n=in.read(buffer,0,left<0?buffer.length:Math.min(buffer.length,left));
                    if(n<0){if(left>0)throw new IOException("Truncated response");break;}
                    append(body,buffer,n);if(left>0)left-=n;
                }
            }
            return result(0,code,body.toByteArray());
        }
    }
    private static String[] endpoint(String target){
        String[] parts=target.split(":",-1);
        if(parts.length!=2||!parts[0].matches("[a-zA-Z0-9][a-zA-Z0-9.-]{0,252}"))
            throw new IllegalArgumentException("Invalid endpoint");
        int port;
        try{port=Integer.parseInt(parts[1]);}catch(NumberFormatException e){throw new IllegalArgumentException("Port");}
        if(port<1||port>65535)throw new IllegalArgumentException("Port");
        permitted(parts[0]);return parts;
    }
    private static int store(Session s){
        long now=System.currentTimeMillis();
        for(Integer id:new ArrayList<>(sessions.keySet()))
            if(now-sessions.get(id).last>SESSION_TTL){sessions.remove(id).close();}
        if(sessions.size()>=MAX_SESSIONS){s.close();throw new IllegalStateException("Too many sessions");}
        for(int i=0;i<65535;i++){
            int id=nextSession++;
            if(nextSession>65535)nextSession=1;
            if(!sessions.containsKey(id)){sessions.put(id,s);return id;}
        }
        s.close();throw new IllegalStateException("Sessions full");
    }
    private static int readU16(InputStream in) throws IOException{
        int a=in.read(),b=in.read();if(a<0||b<0)throw new IOException("EOF");return (a<<8)|b;
    }
    private static byte[] readExact(InputStream in,int length) throws IOException {
        if(length<0||length>LIMIT)throw new TooLarge();
        byte[] out=new byte[length];int offset=0;
        while(offset<length){int n=in.read(out,offset,length-offset);if(n<0)throw new IOException("EOF");offset+=n;}
        return out;
    }
    private static void wsSendFrame(Session s,int opcode,byte[] payload) throws IOException {
        if(payload.length>LIMIT)throw new TooLarge();
        OutputStream out=s.out;
        out.write(0x80|opcode);
        if(payload.length<126)out.write(0x80|payload.length);
        else if(payload.length<65536){out.write(0x80|126);out.write(payload.length>>8);out.write(payload.length);}
        else {out.write(0x80|127);out.write(new byte[]{0,0,0,0,(byte)(payload.length>>>24),
                (byte)(payload.length>>>16),(byte)(payload.length>>>8),(byte)payload.length});}
        byte[] mask=new byte[4];random.nextBytes(mask);out.write(mask);
        byte[] masked=new byte[payload.length];
        for(int i=0;i<payload.length;i++)masked[i]=(byte)(payload[i]^mask[i%4]);
        out.write(masked);out.flush();
    }
    private static Result wsRecv(Session s) throws IOException {
        s.tcp.setSoTimeout(500);
        ByteArrayOutputStream out=new ByteArrayOutputStream();
        try{
            for(int frames=0;frames<128;frames++){
                int b0=s.in.read();if(b0<0)throw new IOException("Closed");
                int b1=s.in.read();if(b1<0||(b1&128)!=0)throw new IOException("Masked server frame");
                int len=b1&127;
                if(len==126)len=readU16(s.in);
                else if(len==127){byte[] wide=readExact(s.in,8);
                    for(int i=0;i<4;i++)if(wide[i]!=0)throw new TooLarge();
                    len=((wide[4]&255)<<24)|((wide[5]&255)<<16)|((wide[6]&255)<<8)|(wide[7]&255);
                }
                if(len<0||len>LIMIT-out.size())throw new TooLarge();
                byte[] chunk=readExact(s.in,len);
                int opcode=b0&15;
                if(opcode==8){s.close();throw new IOException("WebSocket closed");}
                if(opcode==9){wsSendFrame(s,10,chunk);continue;}
                if(opcode==10)continue;
                if(opcode!=0&&opcode!=1&&opcode!=2)throw new IOException("WebSocket opcode");
                out.write(chunk);
                if((b0&128)!=0)return result(0,0,out.toByteArray());
            }
            throw new IOException("Too many fragments");
        }catch(SocketTimeoutException idle){
            if(out.size()!=0)throw new IOException("Interrupted WebSocket frame");
            return ok();
        }
    }
    static final class Event {
        final String kind;
        final int session;
        final byte[] data;
        Event(String kind,int session,byte[] data){this.kind=kind;this.session=session;this.data=data;}
    }
    static synchronized Event pollEvent(){
        for(Map.Entry<Integer,Session> entry:sessions.entrySet()) {
            Session s=entry.getValue();
            if(!s.watch)continue;
            s.last=System.currentTimeMillis();
            try {
                if(s.eventPending==null||s.eventOffset>=s.eventPending.length) {
                    s.eventPending=null;s.eventOffset=0;
                    byte[] data;
                    if(s.kind.equals("WS"))data=wsRecv(s).data;
                    else {
                        byte[] buf=new byte[4096];int n;
                        if(s.kind.equals("UDP")) {
                            s.udp.setSoTimeout(100);
                            DatagramPacket packet=new DatagramPacket(buf,buf.length);
                            s.udp.receive(packet);n=packet.getLength();
                        }else {
                            s.tcp.setSoTimeout(100);
                            n=s.in.read(buf);
                            if(n<0)throw new IOException("Socket closed");
                        }
                        data=Arrays.copyOf(buf,n);
                    }
                    if(data.length==0)continue;
                    s.eventPending=data;
                }
                int n=Math.min(4096,s.eventPending.length-s.eventOffset);
                byte[] part=Arrays.copyOfRange(s.eventPending,s.eventOffset,s.eventOffset+n);
                s.eventOffset+=n;
                s.last=System.currentTimeMillis();
                return new Event(s.kind,entry.getKey(),part);
            }catch(SocketTimeoutException idle){/* No unsolicited data right now. */}
            catch(Exception error){s.watch=false;s.close();}
        }
        return null;
    }
    private static Result socketOperation(String verb,String target,byte[] data) throws IOException {
        String[] action=verb.split("_",-1);
        if(action.length!=2)throw new IllegalArgumentException("Verb");
        String kind=action[0],operation=action[1];
        if(operation.equals("OPEN")){
            if(data.length>0)throw new IllegalArgumentException("OPEN data");
            if(kind.equals("WS")){
                URL u=url(target,"WS");
                String host=u.getHost();boolean secure=u.getProtocol().equals("https");
                int port=u.getPort()==-1?(secure?443:80):u.getPort();
                Socket socket=connect(host,port,secure);
                try{
                    String resource=u.getFile().isEmpty()?"/":u.getFile();
                    byte[] nonce=new byte[16];random.nextBytes(nonce);
                    String key=Base64.getEncoder().encodeToString(nonce);
                    String authority=host+((secure&&port==443)||(!secure&&port==80)?"":":"+port);
                    String handshake="GET "+resource+" HTTP/1.1\r\nHost: "+authority+
                        "\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: "+key+"\r\n\r\n";
                    socket.getOutputStream().write(handshake.getBytes(StandardCharsets.US_ASCII));
                    InputStream input=socket.getInputStream();
                    if(!readLine(input).startsWith("HTTP/1.1 101 "))throw new IOException("WebSocket upgrade rejected");
                    String accept="";
                    for(int i=0;i<128;i++){
                        String line=readLine(input);if(line.isEmpty())break;
                        int colon=line.indexOf(':');
                        if(colon>0&&line.substring(0,colon).equalsIgnoreCase("Sec-WebSocket-Accept"))
                            accept=line.substring(colon+1).trim();
                    }
                    byte[] digest=MessageDigest.getInstance("SHA-1").digest((key+
                        "258EAFA5-E914-47DA-95CA-C5AB0DC85B11").getBytes(StandardCharsets.US_ASCII));
                    if(!accept.equals(Base64.getEncoder().encodeToString(digest)))
                        throw new IOException("WebSocket handshake mismatch");
                    return result(0,store(new Session(kind,socket,null)),new byte[0]);
                }catch(Exception error){socket.close();if(error instanceof IOException)throw (IOException)error;
                    throw new IOException(error);}
            }
            String[] endpoint=endpoint(target);int port=Integer.parseInt(endpoint[1]);
            if(kind.equals("TCP")){
                Socket socket=connect(endpoint[0],port,false);
                return result(0,store(new Session(kind,socket,null)),new byte[0]);
            }
            if(kind.equals("UDP")){
                DatagramSocket socket=new DatagramSocket();
                socket.connect(new InetSocketAddress(endpoint[0],port));socket.setSoTimeout(500);
                return result(0,store(new Session(kind,null,socket)),new byte[0]);
            }
            throw new IllegalArgumentException("Kind");
        }
        int id;
        try{id=Integer.parseInt(target);}catch(NumberFormatException e){throw new IllegalArgumentException("Session ID");}
        Session s=sessions.get(id);
        if(s==null||!s.kind.equals(kind)||System.currentTimeMillis()-s.last>SESSION_TTL)
            throw new IllegalArgumentException("Unknown session");
        s.last=System.currentTimeMillis();
        if(operation.equals("CLOSE")){sessions.remove(id);s.close();return ok();}
        if(operation.equals("WATCH")) {
            if(data.length!=1 || (data[0]!=0 && data[0]!=1))
                throw new IllegalArgumentException("Watch mode");
            s.watch=data[0]==1;
            s.eventPending=null;s.eventOffset=0;
            return ok();
        }
        if(operation.equals("SEND")){
            if(kind.equals("UDP"))s.udp.send(new DatagramPacket(data,data.length));
            else if(kind.equals("WS"))wsSendFrame(s,2,data);
            else {s.out.write(data);s.out.flush();}
            return ok();
        }
        if(operation.equals("RECV")){
            if(kind.equals("WS"))return wsRecv(s);
            byte[] buffer=new byte[65536];
            try{
                int n;
                if(kind.equals("UDP")){
                    DatagramPacket packet=new DatagramPacket(buffer,buffer.length);
                    s.udp.receive(packet);n=packet.getLength();
                }else {s.tcp.setSoTimeout(500);n=s.in.read(buffer);if(n<0)throw new IOException("Closed");}
                return result(0,0,Arrays.copyOf(buffer,n));
            }catch(SocketTimeoutException idle){return ok();}
        }
        throw new IllegalArgumentException("Operation");
    }
    static synchronized Result execute(NetworkMailbox.Request request,boolean approved){
        if(!approved)return result(7,0,new byte[0]);
        try {
            if(request.data.length>LIMIT)return result(4,0,new byte[0]);
            if(request.verb.equals("HTTPX")) {
                if(!request.target.startsWith("https://"))return result(7,0,new byte[0]);
                HttpX x=decodeHttpX(request.data);
                return http(x.method,request.target,x.body,x.headers);
            }
            if(Arrays.asList("GET","POST","PUT","PATCH","DELETE","HEAD").contains(request.verb))
                return http(request.verb,request.target,request.data);
            return socketOperation(request.verb,request.target,request.data);
        }catch(TooLarge e){return result(4,0,new byte[0]);}
        catch(IllegalArgumentException e){return result(7,0,new byte[0]);}
        catch(SocketTimeoutException e){return result(5,0,new byte[0]);}
        catch(IllegalStateException e){return result(4,0,new byte[0]);}
        catch(Exception e){return result(6,0,new byte[0]);}
    }
    /* Legacy WV1 PING/deny behavior retained for older clients. */
    static byte[] handle(LinkProtocol.Message packet,boolean approved){
        if(packet.action.equals("PING")&&packet.fields.length==1)
            return LinkProtocol.frame("PONG",packet.fields[0]);
        if(packet.fields.length>0)return LinkProtocol.frame("ERROR",packet.fields[0],"DENIED");
        return null;
    }
}
