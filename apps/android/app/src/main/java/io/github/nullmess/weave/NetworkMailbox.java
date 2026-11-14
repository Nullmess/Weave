package io.github.nullmess.weave;

import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

/** WN2 binary mailbox over authenticated Flipper Storage RPC. The marker must
 * only be uploaded after net.response is completely transferred. */
final class NetworkMailbox {
    static final String LINK = "/ext/.weave/net.link";
    static final String REQUEST = "/ext/.weave/net.request";
    static final String RESPONSE = "/ext/.weave/net.response";
    static final String READY = "/ext/.weave/net.response.ready";
    static final int MAX_BODY = 16*1024*1024;
    private static final Set<String> VERBS = new HashSet<>(Arrays.asList(
        "GET","POST","PUT","PATCH","DELETE","HEAD",
        "TCP_OPEN","TCP_SEND","TCP_RECV","TCP_CLOSE",
        "UDP_OPEN","UDP_SEND","UDP_RECV","UDP_CLOSE",
        "WS_OPEN","WS_SEND","WS_RECV","WS_CLOSE"));
    static final class Request {
        final String id, verb, target;
        final byte[] data;
        Request(String id, String verb, String target, byte[] data) {
            this.id=id;this.verb=verb;this.target=target;this.data=data;
        }
    }
    static Request parse(byte[] bytes) {
        if(bytes==null||bytes.length<14||bytes.length>MAX_BODY+300)return null;
        int newline=-1;
        for(int i=0;i<bytes.length&&i<281;i++)if(bytes[i]=='\n'){newline=i;break;}
        if(newline<0)return null;
        String header=new String(bytes,0,newline,StandardCharsets.US_ASCII);
        String[] parts=header.split("\\|",-1);
        if(parts.length!=5||!"WN2".equals(parts[0])||!parts[1].matches("[1-9][0-9]{0,9}")||
            !VERBS.contains(parts[2])||parts[3].length()<1||parts[3].length()>=192||
            !parts[4].matches("[0-9]{1,8}"))return null;
        for(char c:parts[3].toCharArray())if(c<33||c>126||c=='|')return null;
        long id,length;
        try{id=Long.parseLong(parts[1]);length=Long.parseLong(parts[4]);}
        catch(NumberFormatException error){return null;}
        if(id>0xffffffffL||length>MAX_BODY||newline+1+length!=bytes.length)return null;
        if(length>0&&(parts[2].equals("GET")||parts[2].equals("HEAD")||
            !(parts[2].endsWith("SEND")||parts[2].equals("POST")||
              parts[2].equals("PUT")||parts[2].equals("PATCH")||parts[2].equals("DELETE"))))return null;
        return new Request(parts[1],parts[2],parts[3],Arrays.copyOfRange(bytes,newline+1,bytes.length));
    }
    static byte[] heartbeat(String type,long sequence) {
        if((!"USB".equals(type)&&!"BLE".equals(type))||sequence<0||sequence>0xffffffffL)
            throw new IllegalArgumentException("Invalid heartbeat");
        return ("WN1|"+type+"|"+sequence+"\n").getBytes(StandardCharsets.US_ASCII);
    }
    static byte[] result(String id,NetworkGateway.Result result) {
        if(!id.matches("[1-9][0-9]{0,9}")||result.status<0||result.status>7||
            result.code<0||result.code>65535||result.data.length>MAX_BODY)
            throw new IllegalArgumentException("Invalid result");
        byte[] header=("WN2|"+id+"|"+result.status+"|"+result.code+"|"+
            result.data.length+"\n").getBytes(StandardCharsets.US_ASCII);
        byte[] combined=Arrays.copyOf(header,header.length+result.data.length);
        System.arraycopy(result.data,0,combined,header.length,result.data.length);
        return combined;
    }
    static byte[] ready(String id){
        if(!id.matches("[1-9][0-9]{0,9}"))throw new IllegalArgumentException("ID");
        return ("WN2|"+id+"\n").getBytes(StandardCharsets.US_ASCII);
    }
}
