#!/usr/bin/env python3
"""Régression JVM : politique FW, vrai parseur TGZ/RPC et ordonnanceur du service simulé.
Ne remplace pas un build Android ni un test de transfert sur matériel.
"""
import io
from pathlib import Path
import subprocess
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[4]
JAVA = ROOT / 'apps/android/app/src/main/java/io/github/nullmess/weave'
SERVICE = (JAVA / 'WeaveService.java').read_text()

# Extrait le job réel du service Android pour le tester sans SDK ni GATT.
JOB = SERVICE[SERVICE.index('    private static final class FirmwareJob {'):
              SERVICE.index('    private final ArrayDeque<String> queuedDeletes')]
REPLY = SERVICE[SERVICE.index('    private static final class FirmwareReply {'):
                SERVICE.index('    private void onFindDeviceRequestRead')]

HARNESS = r'''
package io.github.nullmess.weave;
import java.io.*;
import java.util.*;
import java.util.concurrent.*;
final class FirmwareServiceTest {
    static class android { static class os { static class SystemClock {
        static long now=0; static long elapsedRealtime(){return now;}
    } } }
    static class Intent {
        Intent(Object... ignored){} Intent setPackage(String s){return this;}
        Intent putExtra(String k,Object v){return this;} String getStringExtra(String k){return null;}
    }
    static class Prefs {
        Map<String,Object> values=new HashMap<>();
        boolean getBoolean(String k,boolean d){return (Boolean)values.getOrDefault(k,d);}
        String getString(String k,String d){return (String)values.getOrDefault(k,d);}
        int getInt(String k,int d){return (Integer)values.getOrDefault(k,d);}
        Prefs edit(){return this;} Prefs putString(String k,String v){values.put(k,v);return this;}
        Prefs putInt(String k,int v){values.put(k,v);return this;} void apply(){}
    }
    static class Handler {
        record Task(Runnable r,long due,long seq){}
        long seq; PriorityQueue<Task> q=new PriorityQueue<>(Comparator.comparingLong(Task::due).thenComparingLong(Task::seq));
        void post(Runnable r){postDelayed(r,0);} void postDelayed(Runnable r,long t){q.add(new Task(r,android.os.SystemClock.now+t,seq++));}
        void removeCallbacks(Runnable r){q.removeIf(t->t.r==r);}
        void advance(long duration){long end=android.os.SystemClock.now+duration;int count=0;
            while(!q.isEmpty()&&q.peek().due<=end){if(++count>10000)throw new AssertionError("busy loop");Task t=q.remove();android.os.SystemClock.now=t.due;t.r.run();}android.os.SystemClock.now=end;}
    }
    static class WeaveUpdates {
        record Release(String version){} // field below matches real class (not record)
    }
'''.replace('    static class WeaveUpdates {\n        record Release(String version){} // field below matches real class (not record)\n    }','''    static class WeaveUpdates {
        static class Release { final String version; Release(String v){version=v;} }
        static File archive; static String published="1.0.3"; static int downloads;
        static Release latest(boolean firmware,boolean preview){return new Release(published);}
        static int compare(String a,String b){return FirmwareUpdatePolicy.compare(a,b);}
        static File download(Object owner,Release r,boolean fw,java.util.function.IntConsumer cb){downloads++;cb.accept(100);return archive;}
    }''')
HARNESS += r'''
    final Handler handler=new Handler(); final Prefs prefs=new Prefs();
    final class DirectWorker { Future<?> submit(Runnable r){FutureTask<Void> f=new FutureTask<>(r,null);f.run();return f;} }
    final DirectWorker firmwareNetwork=new DirectWorker();
    final class Link {
        boolean idle=true, priority, ready=true; String key="BLE:one", uid="AABBCCDD", installed="1.0.2", fail;
        List<String> commands=new ArrayList<>();
        boolean isRpcReady(){return ready;} boolean isIdleForFirmware(){return idle;}
        String connectedDeviceKey(){return key;} String deviceUid(){return uid;} String deviceFirmwareVersion(){return installed;}
        void setFirmwarePriority(boolean p){priority=p;} void requestDeviceInfo(){}
        void startScreen(){} void requestInventory(){} void requestStorageInfo(String path){} void refreshDeviceInfo(){}
        void op(String label){if(!idle)throw new AssertionError("RPC collision "+label);idle=false;commands.add(label);
            handler.post(()->{idle=true;handleFirmwareOperation(label,!label.equals(fail),label.equals(fail)?"test failure":null);});}
        void firmwareMkdir(String path,String label){op(label);} void uploadFile(String path,byte[] data,String label){op(label);}
        void prepareFirmwareUpdate(String path){op("Firmware Prepare");} void rebootForFirmwareUpdate(){op("Firmware Reboot");}
    }
    final Link ble=new Link(); boolean bleConfirmed=true, stopping;
    boolean shopNetworkBusy,shopTransferActive,shopVerifying,deletingBatch,catalogCleanupRunning;
    Object pendingShopFap,pendingInstallPath,activeCatalogJob,pendingRemoteInstallFinal,nativeShopHomeToken,shopRequestToken;
    String shopPhase="";
    final Runnable drainCatalogQueue=()->{}, ringDrain=()->{}, shopProcess=()->{};
    final String EVENT_FIRMWARE_UPDATE="event", KEY_PREVIEW_UPDATES="preview", FW_INSTALL_RESULT="/ext/.weave/fw-install.result", FW_CHECK_RESULT="/ext/.weave/fw-check.result";
    String lastFirmwareCheckToken,lastFirmwareCheckVersion,lastFirmwareCheckDevice,lastFirmwareCheckNonce,lastFirmwareInstallNonce;
    File cache;
    FirmwareServiceTest(File cache){this.cache=cache;}
    File getCacheDir(){return cache;} String getPackageName(){return "test";} void sendBroadcast(Intent e){}
'''
HARNESS += JOB + REPLY
HARNESS += r'''
    static int checks;
    static void expect(boolean ok,String name){checks++;if(!ok)throw new AssertionError(name);}
    String phase(){return prefs.getString("firmware_update_phase", "");}
    void verifyBoot(String uid,String version){
        FirmwareJob job=firmwareJob;
        if(job!=null && "rebooting".equals(job.phase) && job.disconnectedForReboot &&
                FirmwareUpdatePolicy.verified(job.uid,uid,job.version,version))finishFirmwareJob(true,"verified");
    }
    static void mainTest(File root,File good) throws Exception {
        WeaveUpdates.archive=good;
        FirmwareServiceTest f=new FirmwareServiceTest(new File(root,"first"));
        f.ble.idle=false;
        f.startFirmwareDownloadInstall("1.0.3",null);
        f.handler.advance(2000);
        expect(f.ble.commands.isEmpty() && f.ble.priority && "waiting".equals(f.phase()),"wait for busy RPC");
        FirmwareJob first=f.firmwareJob;
        f.startFirmwareDownloadInstall("1.0.3",null);
        expect(first==f.firmwareJob,"double tap is idempotent");
        f.ble.idle=true; f.handler.advance(5000);
        expect(f.ble.commands.equals(List.of("Firmware Mkdir Root","Firmware Mkdir Package","Firmware Upload","Firmware Upload","Firmware Upload","Firmware Prepare","Firmware Reboot")),"ordered upload and prepare");
        expect("rebooting".equals(f.phase()),"not success on upload");
        f.verifyBoot(f.ble.uid,"1.0.3"); expect(f.firmwareJob!=null,"must observe reconnect");
        f.firmwareJob.disconnectedForReboot=true;
        f.verifyBoot("OTHER","1.0.3");expect(f.firmwareJob!=null,"wrong device not success");
        f.verifyBoot(f.ble.uid,"1.0.30");expect(f.firmwareJob!=null,"wrong version not success");
        f.verifyBoot(f.ble.uid,"1.0.3");expect(f.firmwareJob==null&&"ready".equals(f.phase())&&!f.ble.priority,"verified reboot and unlock");
        FirmwareServiceTest n=new FirmwareServiceTest(new File(root,"native"));n.ble.key="USB:device";
        n.lastFirmwareCheckToken="123-456";n.lastFirmwareCheckVersion="1.0.3";n.lastFirmwareCheckDevice=n.ble.key;
        n.lastFirmwareCheckNonce="0123456789abcdef01234567";n.firmwareCheckNonceAt=android.os.SystemClock.now;
        byte[] request=(n.lastFirmwareCheckToken+"|1.0.3|"+n.lastFirmwareCheckNonce).getBytes(java.nio.charset.StandardCharsets.US_ASCII);
        n.onFlipperFirmwareInstall(request);n.handler.advance(5000);
        expect(n.ble.commands.get(0).equals("Firmware Accept"),"native ACK before upload");
        expect(n.ble.commands.get(n.ble.commands.size()-1).equals("Firmware Reboot"),"USB native path reaches reboot");
        int previous=n.ble.commands.size();n.onFlipperFirmwareInstall(request);n.handler.advance(100);expect(n.ble.commands.size()==previous,"nonce replay ignored");
        FirmwareServiceTest fail=new FirmwareServiceTest(new File(root,"failed"));fail.ble.fail="Firmware Upload";
        fail.startFirmwareDownloadInstall("1.0.3",null);fail.handler.advance(5000);
        expect("error".equals(fail.phase()) && !fail.ble.commands.contains("Firmware Reboot")&&!fail.ble.priority,"upload failure cannot reboot");
        FirmwareServiceTest timeout=new FirmwareServiceTest(new File(root,"timeout"));timeout.ble.idle=false;
        timeout.startFirmwareDownloadInstall("1.0.3",null);timeout.handler.advance(61000);
        expect(timeout.firmwareJob==null&&"error".equals(timeout.phase())&&!timeout.ble.priority,"bounded idle wait");
        FirmwareServiceTest disconnect=new FirmwareServiceTest(new File(root,"disconnect"));disconnect.ble.idle=false;
        disconnect.startFirmwareDownloadInstall("1.0.3",null);disconnect.ble.key="OTHER";disconnect.handler.advance(1000);
        expect("error".equals(disconnect.phase())&&disconnect.ble.commands.isEmpty(),"no cross-device transfer");
        FirmwareServiceTest reply=new FirmwareServiceTest(new File(root,"reply"));reply.ble.idle=false;
        reply.lastFirmwareCheckDevice=reply.ble.key;reply.lastFirmwareCheckToken="123-456";
        reply.writeFirmwareCheckResult("123-456|ok|1.0.3".getBytes(),0);reply.handler.advance(1000);
        expect(reply.ble.priority&&reply.ble.commands.isEmpty(),"prioritized mailbox reply");
        reply.ble.idle=true;reply.handler.advance(1000);
        expect(reply.ble.commands.equals(List.of("Firmware Check"))&&!reply.ble.priority,"mailbox completion releases RPC");
    }
    public static void main(String[] args)throws Exception {
        File root=new File(args[0]),good=new File(root,"good.tgz");
        var packageFiles=WeaveFirmwarePackage.extract(good,new File(root,"stage"),"1.0.3");
        expect(packageFiles.files.size()==3&&packageFiles.files.get(2).remotePath.endsWith("update.fuf"),"manifest uploaded last");
        for(String name:List.of("bad_target","missing_member","traversal","bad_version")){
            boolean rejected=false;try{WeaveFirmwarePackage.extract(new File(root,name+".tgz"),new File(root,name),"1.0.3");}catch(IOException e){rejected=true;}
            expect(rejected,"reject "+name);
        }
        expect(FirmwareUpdatePolicy.compare("1.0.10","1.0.3")>0,"numeric versions");
        expect(FirmwareUpdatePolicy.versionIn("Weave v1.0.3").equals("1.0.3"),"parse installed version");
        expect(FirmwareUpdatePolicy.uploadPercent(0,100)==15&&FirmwareUpdatePolicy.uploadPercent(100,100)==98,"byte-weighted progress");
        expect(!FirmwareUpdatePolicy.authorized(new String[]{"123-456","1.0.3","0123456789abcdef01234567"},"A","A","123-456","1.0.3","0123456789abcdef01234567",null,600001),"expired nonce");
        byte[] msg=FlipperRpc.systemUpdateRequest(18,packageFiles.manifestPath);
        int[] prefix={0}; int length=FlipperRpc.frameLength(msg,0,prefix);
        expect(length>0&&FlipperRpc.parseReply(Arrays.copyOfRange(msg,prefix[0],msg.length)).type==FlipperRpc.SYSTEM_UPDATE_REQUEST,"prepare RPC encoding");
        mainTest(root,good);
        System.out.println("Firmware update JVM: "+checks+" checks passed (simulated RPC, no physical device)");
    }
}
'''
# Run real parser on USTAR fixtures with the same naming convention as FBT.
def make_bundle(path, target='7', version='1.0.3', missing=False, traversal=False):
    with tarfile.open(path, 'w:gz', format=tarfile.USTAR_FORMAT) as tar:
        root='f7-update-1.0.3'
        manifest=f'Filetype: Flipper firmware upgrade configuration\nVersion: 2\nInfo: {version}\nTarget: {target}\nLoader: updater.bin\nFirmware: firmware.dfu\n'.encode()
        files=[('update.fuf',manifest),('updater.bin',b'loader'*20)]
        if not missing: files.append(('firmware.dfu',bytes(range(256))*4))
        if traversal: files.append(('../evil',b'x'))
        for name,data in files:
            entry=tarfile.TarInfo(root+'/'+name);entry.size=len(data);tar.addfile(entry,io.BytesIO(data))

with tempfile.TemporaryDirectory(prefix='weave-fw-test-') as tmp:
    p=Path(tmp)
    make_bundle(p/'good.tgz');make_bundle(p/'bad_target.tgz',target='6');make_bundle(p/'missing_member.tgz',missing=True)
    make_bundle(p/'traversal.tgz',traversal=True);make_bundle(p/'bad_version.tgz',version='1.0.4')
    harness=p/'FirmwareServiceTest.java';harness.write_text(HARNESS)
    subprocess.run(['javac','--release','17','-d',str(p),str(harness),*[str(JAVA/f) for f in ('FirmwareUpdatePolicy.java','WeaveFirmwarePackage.java','FlipperRpc.java')]],check=True)
    subprocess.run(['java','-cp',str(p),'io.github.nullmess.weave.FirmwareServiceTest',str(p)],check=True)
