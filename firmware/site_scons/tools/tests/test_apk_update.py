#!/usr/bin/env python3
"""Régression JVM du flux APK et cohérence des exports SDK/README (sans Android)."""
import csv
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[4]
JAVA = ROOT / 'apps/android/app/src/main/java/io/github/nullmess/weave'
activity = (JAVA / 'MainActivity.java').read_text()
updates = (JAVA / 'WeaveUpdates.java').read_text()
release = updates[updates.index('    static final class Release {'):updates.index('    interface Progress')]
flow = activity[activity.index('    private void openApkInstaller()'):activity.index('    private void offerApkRecovery(')]
permission_result = activity[activity.index('        if (requestCode == REQUEST_UPDATE_APK_PERMISSION) {'):activity.index('        if (requestCode == REQUEST_UPDATE_APK_SAVE) {')]
harness = r'''
import java.io.*;
class ApkUpdateTest {
    static class Uri { String value; Uri(String v){value=v;} static Uri parse(String v){return new Uri(v);} }
    static class Intent {
        static final String ACTION_INSTALL_PACKAGE="install", EXTRA_RETURN_RESULT="return";
        static final int FLAG_GRANT_READ_URI_PERMISSION=1;
        String action, type; Uri uri; int flags; boolean result;
        Intent(String a){action=a;} Intent(String a,Uri u){action=a;uri=u;}
        void setDataAndType(Uri u,String t){uri=u;type=t;}
        void setClipData(Object o){} void addFlags(int f){flags|=f;}
        void putExtra(String k,boolean v){if(EXTRA_RETURN_RESULT.equals(k))result=v;}
    }
    static class Settings {static final String ACTION_MANAGE_UNKNOWN_APP_SOURCES="permission";}
    static class ClipData {static Object newRawUri(String label,Uri uri){return uri;}}
    static class PackageManager {boolean allowed; boolean canRequestPackageInstalls(){return allowed;}}
    static class Prefs {String phase=""; String getString(String k,String fallback){return phase;}}
    static class FirmwareUpdatePolicy {static boolean activePhase(String s){return !s.isEmpty();}}
    static final int REQUEST_UPDATE_APK_PERMISSION=82, REQUEST_UPDATE_APK_INSTALL=83, RESULT_OK=-1;
    final PackageManager pm=new PackageManager(); final Prefs prefs=new Prefs();
    boolean apkReadyToInstall=true,appForeground=true, destroyed, failLaunch;
    int apkUpdateRequest=0, calls=0, recovery=0, clears=0;
    File pendingUpdateApk; Intent last; String message;
    ApkUpdateTest(File f){pendingUpdateApk=f;}
    PackageManager getPackageManager(){return pm;} String getPackageName(){return "io.github.nullmess.weave";}
    boolean isFinishing(){return false;} boolean isDestroyed(){return destroyed;}
    void startActivityForResult(Intent i,int r){if(failLaunch)throw new RuntimeException("test"); calls++;last=i;}
    void clearApkUpdate(){clears++;apkUpdateRequest=0;apkReadyToInstall=false;pendingUpdateApk=null;}
    void offerApkRecovery(String m){recovery++;message=m;}
    void toast(String m){message=m;}
'''+release+flow+'\nvoid result(int requestCode,int resultCode){\n'+permission_result+r'''
    }
    static int checks;
    static void check(boolean b,String s){checks++;if(!b)throw new AssertionError(s);}
    public static void main(String[] args) throws Exception {
        File f=new File(args[0]);
        Release r=new Release("1.0.3","app.apk","url","digest",98765);
        check(r.size==98765,"asset size retained");
        ApkUpdateTest a=new ApkUpdateTest(f);
        a.openApkInstaller();
        check(a.last.action.equals("permission") && a.apkUpdateRequest==82,"unknown-source permission");
        a.openApkInstaller(); check(a.calls==1,"no double launch");
        a.pm.allowed=true; a.appForeground=false; a.result(82,0);
        check(a.apkReadyToInstall && a.calls==1,"defer until foreground");
        a.appForeground=true; a.openApkInstaller();
        check(a.last.action.equals("install") && a.apkUpdateRequest==83,"native installer");
        check(a.last.uri.value.equals("content://io.github.nullmess.weave.updates/weave-app.apk"),"local provider");
        check(a.last.flags==1 && a.last.result,"read grant and result");
        a.result(83,0); check(a.recovery==1 && a.clears==0,"refusal keeps APK");
        a.result(83,-1); check(a.clears==1,"success cleanup");
        ApkUpdateTest b=new ApkUpdateTest(f); b.openApkInstaller(); b.result(82,0);
        check(b.recovery==1 && b.calls==1,"permission denied");
        ApkUpdateTest c=new ApkUpdateTest(f); c.pm.allowed=true; c.prefs.phase="uploading"; c.openApkInstaller();
        check(c.recovery==1 && c.calls==0,"firmware transfer protected");
        ApkUpdateTest d=new ApkUpdateTest(f); d.pm.allowed=true; d.failLaunch=true; d.openApkInstaller();
        check(d.recovery==1 && d.apkUpdateRequest==0,"missing installer recovery");
        ApkUpdateTest e=new ApkUpdateTest(new File(f.getParentFile(),"absent")); e.openApkInstaller();
        check(e.clears==1 && e.calls==0,"missing APK");
        System.out.println("APK update JVM: "+checks+" checks passed (Android API stubs)");
    }
}
'''
with tempfile.TemporaryDirectory() as temp:
    d=Path(temp); (d/'ApkUpdateTest.java').write_text(harness); (d/'app.apk').write_bytes(b'test')
    subprocess.run(['javac',str(d/'ApkUpdateTest.java')],check=True)
    subprocess.run(['java','-cp',str(d),'ApkUpdateTest',str(d/'app.apk')],check=True)
# La table contient exactement les fonctions publiques Weave du SDK, pas les helpers internes.
with (ROOT/'firmware/targets/f7/api_symbols.csv').open() as f:
    exports={r[2]:r[3] for r in csv.reader(f) if len(r)>=4 and r[0]=='Function' and r[1]=='+' and r[2].startswith('weave_')}
readme=(ROOT/'README.md').read_text().split('## 🧑‍💻 Developer API',1)[1].split('\n---',1)[0]
rows=re.findall(r'^\| [^|]+ \| `(weave_\w+)` \| [^|]+ \| `([^`]+)` \|$',readme,re.M)
assert len(rows)==len(exports)==len(dict(rows))
assert {name:'_Bool' if typ=='bool' else typ for name,typ in rows}==exports
assert 'ACTION_DELETE' not in flow and 'ALL_USERS' not in flow
print(f'Developer API: {len(rows)}/{len(exports)} exported functions documented')

# Un seul canal de MAJ, sans option preview ni préférence résiduelle utilisée.
service = (JAVA / 'WeaveService.java').read_text()
assert 'Include pre-releases' not in activity
assert 'KEY_PREVIEW_UPDATES' not in activity + service
assert 'static Release latest(boolean firmware)' in updates
assert '"/releases/latest"' in updates and '/releases?per_page=' not in updates
print('Release channel: stable only')
