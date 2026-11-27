<p align="center">
  <img src="firmware/assets/branding/Weave.png" width="160" alt="Weave logo">
</p>

<h1 align="center">
    <strong>Weave</strong>
</h1>

<p align="center">🧵 Flipper Zero firmware for network access without Wi-Fi hardware.</p>

<p align="center">
    <a href="https://github.com/nullmess/Weave/releases/latest">
        <img src="https://img.shields.io/github/v/release/nullmess/Weave?style=flat&label=release" alt="Release">
    </a>
    <a href="LICENSE">
        <img src="https://img.shields.io/github/license/nullmess/Weave?style=flat" alt="License">
    </a>
    <img src="https://hits.sh/github.com/nullmess/Weave.svg?label=views" alt="Views">
</p>

---

<p align="center">
  <img src=".github/assets/recap.gif" width="235" alt="Weave Android companion preview">
</p>


---

## ✨ Features

- Native FAP networking over USB/Bluetooth.
- Extended USB HID, CCID and Composite support.
- Optimized cryptography and secure storage.
- Application permissions and resource management.
- Native file, app and system management.
- Extended device controls and radio features.
- Companion apps for Android, Windows, Linux and macOS.
- Developer APIs and uFBT-compatible SDK.

---

## 🚀 Commands

### Linux / macOS

```sh
python3 fbt.py build
python3 fbt.py build --firmware-only
python3 fbt.py sdk
python3 fbt.py flash
python3 fbt.py clean
python3 fbt.py doctor --diagnose
python3 fbt.py dev
```

### Windows

```powershell
py -3 fbt.py build
py -3 fbt.py build --firmware-only
py -3 fbt.py sdk
py -3 fbt.py flash
py -3 fbt.py clean
py -3 fbt.py doctor --diagnose
py -3 fbt.py dev
```

Build outputs are available in `build/`.

### SDK Installation

```sh
ufbt update --hw-target=f7 --local=build/Weave-1.0.3-sdk.zip
```

---


## 🧑‍💻 Developer API

| API | Capabilities |
|---|---|
| Networking | HTTP/HTTPS, TCP, UDP, WebSocket, file upload, transport detection |
| USB | HID, CCID, Composite, custom descriptors, data transfer |
| Hashing | SHA-1, SHA-256, SHA-512, HMAC |
| Key Derivation | HKDF, PBKDF2, reusable HMAC contexts |
| Encryption | AES-GCM, AES-CBC, AES-ECB, hardware acceleration |
| Asymmetric Cryptography | ECC, ECDSA, ECDH, RSA, Ed25519, Ed25519ph |
| Cryptographic Jobs | Cooperative execution, progress, cancellation |
| Secure Storage | Encrypted secret storage, retrieval, deletion |
| Security Utilities | Random generation, secure memory wiping, constant-time comparison |
| Permissions | Capability verification, permission management |

### Example

```c
#include <furi.h>
#include <weave/weave_security.h>

int32_t example_app(void* context) {
    UNUSED(context);

    uint8_t hash[32];

    if(weave_security_sha256("Hello Weave", 11, hash)) {
        FURI_LOG_I("Example", "SHA-256 computed successfully");
    }

    return 0;
}
```


---

## 🌲 Repository layout

```text
Weave/
├── apps/
│   ├── android/
│   └── desktop/
├── firmware/
│   ├── applications/
│   ├── assets/
│   ├── furi/
│   ├── lib/
│   ├── targets/
│   └── site_scons/
├── .github/
├── fbt.py
├── VERSION
└── README.md
```

---

## 👤 Author

Developed by [Nullmess](https://github.com/Nullmess).

## 📄 License

Based on [Flipper Zero firmware](https://github.com/flipperdevices/flipperzero-firmware) (GPL-3.0-or-later). See [LICENSE](LICENSE).
