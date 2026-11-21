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

Weave provides a native C networking API for Flipper Zero `.fap` applications through USB/BLE and a connected Android/Desktop companion.

**SDK:** [`weave.h`](firmware/applications/services/weave/weave.h)  
**Include:** `#include <weave/weave.h>`  
**Version:** `WEAVE_API_VERSION=3`  
**Service:** `furi_record_open(RECORD_WEAVE)`

### API Reference

All functions implicitly take `Weave* weave` as their first argument.

| Function | Arguments | Return |
|---|---|---|
| `weave_get_transport` | — | `WeaveTransport` |
| `weave_transport_subscribe` | `cb, ctx` | `bool` |
| `weave_transport_unsubscribe` | `cb, ctx` | `void` |
| `weave_http_get` | `url, cb, ctx` | `WeaveStatus` |
| `weave_http_request` | `method, url, body, length, cb, ctx` | `WeaveStatus` |
| `weave_http_request_ex` | `method, url, headers, count, body, length, cb, ctx` | `WeaveStatus` |
| `weave_http_upload_file` | `method, url, sd_path, cb, ctx` | `WeaveStatus` |
| `weave_http_upload_file_ex` | `method, url, headers, count, sd_path, cb, ctx` | `WeaveStatus` |
| `weave_socket_open` | `type, host, port, cb, ctx` | `WeaveStatus` |
| `weave_socket_send` | `type, session, body, length, cb, ctx` | `WeaveStatus` |
| `weave_socket_receive` | `type, session, cb, ctx` | `WeaveStatus` |
| `weave_socket_close` | `type, session, cb, ctx` | `WeaveStatus` |
| `weave_socket_watch` | `type, session, enabled, cb, ctx` | `WeaveStatus` |
| `weave_socket_subscribe_events` | `event_cb, ctx` | `bool` |
| `weave_socket_unsubscribe_events` | `event_cb, ctx` | `void` |
| `weave_context_release` | `ctx, timeout_ms` | `bool` |

**Callbacks:** `cb` receives `WeaveResponse*`, `event_cb` receives `WeaveSocketEvent*`, and `ctx` is the application's context pointer. HTTP/socket functions return a submission status; asynchronous results arrive through callbacks. Socket OPEN returns its session ID through `response->http_status`.

### Example

```c
#include <furi.h>
#include <weave/weave.h>

static int ctx;

static void on_reply(const WeaveResponse* r, void* user) {
    UNUSED(user);
    if(r->status == WeaveStatusOk && r->size)
        FURI_LOG_I("App", "Received %lu bytes", (unsigned long)r->size);
}

static void fetch(void) {
    Weave* w = furi_record_open(RECORD_WEAVE);
    weave_http_get(w, "https://api.github.com", on_reply, &ctx);
    furi_record_close(RECORD_WEAVE);
}

static bool cleanup(void) {
    Weave* w = furi_record_open(RECORD_WEAVE);
    bool safe = weave_context_release(w, &ctx, 2000);
    furi_record_close(RECORD_WEAVE);
    return safe;
}
```

Call `cleanup()` before unloading the FAP; if it returns `false`, keep the application loaded and retry. Callbacks run asynchronously and response buffers are valid only during their callback.


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
