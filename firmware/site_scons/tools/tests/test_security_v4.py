#!/usr/bin/env python3
"""Host regression for the actual v4 SHA/KDF source (GCC, Python stdlib).
Does not emulate the AES peripheral, the Furi scheduler or firmware loader.
"""
import concurrent.futures
import ctypes as C
import hashlib
import hmac
from pathlib import Path
import random
import subprocess
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / 'applications/services/weave'
P = C.c_void_p
N = C.c_size_t
U = C.c_uint32
B = C.c_bool


def bind(lib, name, args, result=B):
    f = getattr(lib, 'weave_security_' + name)
    f.argtypes = args
    f.restype = result
    return f


class Security(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        base = Path(cls.temp.name)
        # Compile the exact wipe/equality implementations, excluding hardware and vault.
        original = (SOURCE / 'weave_security.c').read_text()
        (base/'helper.c').write_text('#include "weave_security.h"\n' +
            original[original.index('void weave_security_wipe('):original.index('bool weave_security_random(')] +
            '\nsize_t test_job_size(void) { return sizeof(WeaveSecurityJob); }\nsize_t test_mac_size(void) { return sizeof(WeaveSecurityHmacSha256Context); }\n'
            'bool test_work_wiped(const WeaveSecurityJob* job) {\n'
            ' const uint8_t* p = (const uint8_t*)&job->work;\n'
            ' for(size_t i=0;i<sizeof(job->work);i++) if(p[i]) return false;\n'
            ' return true; }\n')
        subprocess.run(['gcc','-std=c11','-Os','-Wall','-Wextra','-Werror','-shared','-fPIC',
                        '-fsanitize=undefined','-fno-sanitize-recover=all',
                        '-I',str(SOURCE),str(SOURCE/'weave_security_kdf.c'),str(base/'helper.c'),
                        '-o',str(base/'test.so')],check=True)
        lib = C.CDLL(str(base/'test.so'))
        lib.test_job_size.restype = N
        lib.test_mac_size.restype = N
        lib.test_work_wiped.argtypes = [P]
        lib.test_work_wiped.restype = B
        cls.lib = lib
        cls.sha = staticmethod(bind(lib,'sha256',[P,N,P]))
        cls.mac = staticmethod(bind(lib,'hmac_sha256',[P,N,P,N,P]))
        cls.hkdf = staticmethod(bind(lib,'hkdf_sha256',[P,N,P,N,P,N,P,N]))
        cls.kdf = staticmethod(bind(lib,'pbkdf2_sha256',[P,N,P,N,U,P,N]))
        cls.begin = staticmethod(bind(lib,'job_begin_pbkdf2',[P,P,N,P,N,U,N]))
        cls.begin_sha = staticmethod(bind(lib,'job_begin_sha256',[P,P,N]))
        cls.begin_hmac = staticmethod(bind(lib,'job_begin_hmac_sha256',[P,P,N,P,N]))
        cls.step = staticmethod(bind(lib,'job_step',[P,U],C.c_int))
        cls.progress = staticmethod(bind(lib,'job_progress',[P,P,P]))
        cls.result = staticmethod(bind(lib,'job_result',[P,P,N]))
        cls.cancel = staticmethod(bind(lib,'job_cancel',[P],None))
        cls.api = staticmethod(bind(lib,'api_version',[],U))
        cls.job_size = lib.test_job_size()
        cls.mac_size = lib.test_mac_size()
        cls.mac_init = staticmethod(bind(lib,'hmac_sha256_init',[P,P,N]))
        cls.mac_compute = staticmethod(bind(lib,'hmac_sha256_compute',[P,P,N,P]))
        cls.mac_clear = staticmethod(bind(lib,'hmac_sha256_clear',[P],None))

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def job(self):
        return (C.c_uint64 * ((self.job_size+7)//8))()

    def get_progress(self, job):
        done,total = C.c_uint64(),C.c_uint64()
        self.assertTrue(self.progress(job,C.byref(done),C.byref(total)))
        return done.value,total.value

    def test_sha_hmac(self):
        rng = random.Random(42)
        lengths = [0,1,31,32,55,56,57,63,64,65,119,120,127,128,129,256,4096]
        count=0
        for size in lengths:
            data = rng.randbytes(size)
            out = C.create_string_buffer(32)
            self.assertTrue(self.sha(data,size,out))
            self.assertEqual(out.raw,hashlib.sha256(data).digest());count+=1
            for kn in [0,1,32,63,64,65,128,257]:
                key = rng.randbytes(kn)
                self.assertTrue(self.mac(key,kn,data,size,out))
                self.assertEqual(out.raw,hmac.digest(key,data,'sha256'));count+=1
        print(f'SHA/HMAC vectors: {count}')

    def test_reusable_hmac(self):
        rng=random.Random(46); count=0
        for kn in [0,1,64,65,129]:
            ctx=(C.c_uint32*((self.mac_size+3)//4))(); key=rng.randbytes(kn)
            self.assertTrue(self.mac_init(ctx,key,kn))
            def compute(i):
                data=bytes([i])*i; out=C.create_string_buffer(32)
                return self.mac_compute(ctx,data,len(data),out) and out.raw==hmac.digest(key,data,'sha256')
            with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
                self.assertTrue(all(pool.map(compute,range(64))));count+=64
            self.mac_clear(ctx)
            self.assertFalse(any(bytes(ctx)))
            self.assertFalse(self.mac_compute(ctx,b'x',1,C.create_string_buffer(32)))
        print(f'Reusable HMAC concurrent operations: {count}; clear and use-after-clear rejection: PASS')

    def test_pbkdf2(self):
        rng = random.Random(43);count=0
        for kn in [1,6,32,64,65,130]:
            key = rng.randbytes(kn)
            for sn in [0,1,16,51,52,55,56,60,63,64]:
                salt = rng.randbytes(sn)
                for length in [1,31,32,33,64]:
                    out = C.create_string_buffer(length)
                    self.assertTrue(self.kdf(key,kn,salt,sn,10000,out,length))
                    self.assertEqual(out.raw,hashlib.pbkdf2_hmac('sha256',key,salt,10000,length));count+=1
        key,salt=b'000001',bytes(range(16))
        out=C.create_string_buffer(32)
        self.assertTrue(self.kdf(key,len(key),salt,len(salt),150000,out,32))
        self.assertEqual(out.raw,hashlib.pbkdf2_hmac('sha256',key,salt,150000));count+=1
        print(f'PBKDF2 vectors: {count}')

    def test_hkdf(self):
        rng=random.Random(44);count=0
        for kn in [1,22,64,65,131]:
            key=rng.randbytes(kn)
            for sn in [0,13,64,65]:
                salt=rng.randbytes(sn)
                for n in [0,1,31,32,33,42,64,127,8160]:
                    info=rng.randbytes(65)
                    prk=hmac.digest(salt or bytes(32),key,'sha256')
                    last=b'';expected=b''
                    for i in range(1,(n+31)//32+1):
                        last=hmac.digest(prk,last+info+bytes([i]),'sha256');expected+=last
                    out=C.create_string_buffer(n)
                    self.assertTrue(self.hkdf(key,len(key),salt,len(salt),info,len(info),out,n))
                    self.assertEqual(out.raw,expected[:n]);count+=1
        print(f'HKDF vectors: {count}')

    def test_jobs(self):
        rng=random.Random(45);count=0
        for size in [0,1,55,56,64,65,128,4096]:
            data=rng.randbytes(size);key=rng.randbytes(100)
            for keyed in [False,True]:
                job=self.job();out=C.create_string_buffer(32)
                if keyed:
                    self.assertTrue(self.begin_hmac(job,key,len(key),data,size))
                else:self.assertTrue(self.begin_sha(job,data,size))
                self.assertFalse(self.result(job,out,32))
                self.assertEqual(self.step(job,0),1)
                while True:
                    state=self.step(job,17)
                    done,total=self.get_progress(job)
                    self.assertLessEqual(done,total);self.assertEqual(total,size)
                    if state==2:break
                    self.assertEqual(state,1)
                self.assertTrue(self.lib.test_work_wiped(job))
                self.assertTrue(self.result(job,out,32))
                self.assertEqual(out.raw,hmac.digest(key,data,'sha256') if keyed else hashlib.sha256(data).digest())
                self.assertFalse(any(bytes(job)));count+=1
        for size in [1,32,33,64]:
            job=self.job();out=C.create_string_buffer(size)
            self.assertTrue(self.begin(job,b'pass',4,b'salt',4,10000,size))
            total=10000*((size+31)//32);previous=0
            while True:
                state=self.step(job,123)
                done,t=self.get_progress(job)
                self.assertEqual(t,total);self.assertEqual(done,min(previous+123,total));previous=done
                if state==2:break
            self.assertTrue(self.lib.test_work_wiped(job))
            self.assertTrue(self.result(job,out,size))
            self.assertEqual(out.raw,hashlib.pbkdf2_hmac('sha256',b'pass',b'salt',10000,size))
            self.assertFalse(any(bytes(job)));count+=1
        for init in [lambda j:self.begin(j,b'pass',4,b'salt',4,10000,64),
                     lambda j:self.begin_hmac(j,b'key',3,b'message',7)]:
            job=self.job();self.assertTrue(init(job));self.step(job,2);self.cancel(job)
            self.assertEqual(self.step(job,100),3)
            self.assertTrue(self.lib.test_work_wiped(job));self.assertFalse(self.result(job,C.create_string_buffer(64),64))
        print(f'Resumable jobs: {count}; cancellation: 2')

    def test_invalid_and_concurrent(self):
        out=C.create_string_buffer(64);job=self.job()
        self.assertEqual(self.api(),4)
        self.assertFalse(self.begin(job,b'pass',4,b'salt',4,9999,32))
        self.assertFalse(self.begin(job,b'pass',4,bytes(65),65,10000,32))
        self.assertFalse(self.begin(job,b'pass',4,b'salt',4,10000,65))
        self.assertFalse(self.sha(None,1,out));self.assertFalse(self.mac(None,1,b'',0,out))
        self.assertFalse(self.hkdf(b'k',1,None,0,None,0,out,8161))
        def task(i):
            p=bytes([i])*70;s=bytes([255-i])*64;o=C.create_string_buffer(64)
            return self.kdf(p,len(p),s,len(s),10000,o,64) and o.raw==hashlib.pbkdf2_hmac('sha256',p,s,10000,64)
        with concurrent.futures.ThreadPoolExecutor(max_workers=8) as executor:
            self.assertTrue(all(executor.map(task,range(32))))
        print('Concurrent independent KDF calls: 32; invalid parameter checks: PASS')

    def test_benchmark(self):
        p=b'password';s=b'salt';out=C.create_string_buffer(32);samples=[]
        for _ in range(5):
            start=time.perf_counter();self.assertTrue(self.kdf(p,len(p),s,len(s),150000,out,32))
            samples.append(time.perf_counter()-start)
        print(f'Host PBKDF2 150000: {sorted(samples)[2]*1000:.1f} ms (median); job: {self.job_size} bytes')

if __name__=='__main__':
    unittest.main(verbosity=2)
