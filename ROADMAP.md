# Roadmap

## [1.0.3]

- [x] Security API performance optimizations (host regression tested).
- [x] ARM Cortex-M4 optimized cryptographic primitives (ARM object compilation).
- [x] Reusable cryptographic contexts and reduced allocation overhead.
- [x] Optimized HMAC and PBKDF2 implementations.
- [ ] Hardware-accelerated cryptography where supported (AES-GCM implementation ready; physical validation pending).
- [x] Generic cooperative cryptographic jobs with progress and cancellation (SHA-256, HMAC-SHA256, PBKDF2).
- [ ] Cryptographic performance, memory and stack benchmarks (CLI implemented; on-device measurements pending).
- [x] Cryptographic thread safety and resource management (independent host jobs tested).
- [ ] Secure memory handling and side-channel resistance review (source review, sanitizer and HAL cleanup tests performed; target review pending).
- [ ] Security API backward compatibility and regression testing (old ABI preserved; generic smoke FAPs and CI gate implemented; physical integration pending).
- [ ] uFBT-compatible SDK packaging and distribution (structural tests passed; isolated real SDK/FAP validation runner implemented; real installation/build pending).
- [x] Dedicated SDK build command and release artifacts (build and CI integration implemented).
- [ ] Firmware updates over USB/Bluetooth from Android and Flipper Settings (physical validation pending).

- [ ] Android APK installation in the current profile (physical validation pending).

Release validation requires a complete ARM firmware build, real uFBT build and launch, flash/RAM checks, hardware benchmarks and device regression testing. Do not mark the release complete until these pass.
