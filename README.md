<p align="center">
  <img src="firmware/assets/branding/Weave.png" width="160" alt="Weave logo">
</p>

<p align="center"><strong>Weave</strong></p>

<p align="center">🧵 Flipper Zero firmware for network access without Wi-Fi hardware.</p>

---

<center>
<img src=".github/assets/recap.gif" width="200" alt="Weave Android companion preview">
</center>


---

## ✨ Features

- Native networking for FAPs over USB/Bluetooth.
- File, app and system managementon Flipper Zero.
- Companion apps for Android, Windows, Linux and macOS.
- Extended device controls and radio features.

---

## 🚀 Commands

Linux / macOS:

```sh
python3 fbt.py build
python3 fbt.py flash
python3 fbt.py clean
python3 fbt.py doctor --diagnose
python3 fbt.py dev
```

Windows: use `py -3` instead of `python3`.

Build outputs appear directly in `build/`. `clean` (or `clear`) restores the source tree.

---

## 🧑‍💻 Developer API

Weave exposes a native C API to `.fap` applications without a separate network module.

Header: [`weave.h`](firmware/applications/services/weave/weave.h) · `WEAVE_API_VERSION=2`

| API | Purpose |
|---|---|
| `weave_get_transport` | USB/BLE gateway status |
| `weave_http_get`, `weave_http_request` | HTTP(S) requests |
| `weave_http_upload_file` | Stream SD files |
| `weave_socket_*` | TCP, UDP and WebSocket sessions |

```c
#include <furi.h>
#include <weave/weave.h>

static void on_response(const WeaveResponse* response, void* context) {
    UNUSED(context);
    if(response->status == WeaveStatusOk && !response->more) {
        FURI_LOG_I("Weave", "Received %lu bytes", (unsigned long)response->total);
    }
}

static void request_data(void) {
    Weave* weave = furi_record_open(RECORD_WEAVE);
    weave_http_get(weave, "https://api.github.com", on_response, NULL);
    furi_record_close(RECORD_WEAVE);
}
```

Responses arrive asynchronously in chunks; process `response->data` during the callback. A connected companion and enabled network gateway are required.

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
