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

Weave provides native C networking, USB, security and permission APIs for Flipper Zero `.fap` applications. Networking uses USB/BLE and a connected Android/Desktop companion.

**SDK:** [`weave.h`](firmware/applications/services/weave/weave.h), [`weave_usb.h`](firmware/applications/services/weave/weave_usb.h), [`weave_security.h`](firmware/applications/services/weave/weave_security.h), [`weave_permissions.h`](firmware/applications/services/weave/weave_permissions.h)  
**Include:** `#include <weave/weave.h>` (or the corresponding header under `weave/`)  
**Version:** `WEAVE_API_VERSION=3`, `WEAVE_SECURITY_API_VERSION=3`, `WEAVE_USB_API_VERSION=1`, `WEAVE_PERMISSIONS_API_VERSION=1`  
**Service:** `furi_record_open(RECORD_WEAVE)` (network API only)

### API Reference

Networking functions implicitly take `Weave* weave` as their first argument. USB, security and permission functions take only the arguments listed below.

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
| `weave_usb_acquire_ex` | `owner, mode, hid_report_descriptor, hid_descriptor_size, report_size` | `WeaveUsbResult` |
| `weave_usb_acquire` | `owner, mode` | `WeaveUsbResult` |
| `weave_usb_release` | `owner` | `WeaveUsbResult` |
| `weave_usb_receive` | `owner, channel, buffer, capacity, size` | `WeaveUsbResult` |
| `weave_usb_transmit` | `owner, channel, buffer, size` | `WeaveUsbResult` |
| `weave_usb_ccid_notify` | `owner, present` | `WeaveUsbResult` |
| `weave_usb_connected` | `owner` | `bool` |
| `weave_usb_cdc_available` | — | `bool` |
| `weave_security_random` | `out, size` | `bool` |
| `weave_security_wipe` | `data, size` | `void` |
| `weave_security_equal` | `a, b, size` | `bool` |
| `weave_security_sha256` | `data, size, out` | `bool` |
| `weave_security_hmac_sha256` | `key, key_len, data, size, out` | `bool` |
| `weave_security_hkdf_sha256` | `key, key_size, salt, salt_size, info, info_size, out, out_size` | `bool` |
| `weave_security_pbkdf2_sha256` | `password, password_size, salt, salt_size, iterations, out, out_size` | `bool` |
| `weave_security_aead_encrypt` | `key, nonce, aad, aad_size, plaintext, size, ciphertext, tag` | `bool` |
| `weave_security_aead_decrypt` | `key, nonce, aad, aad_size, ciphertext, size, tag, plaintext` | `bool` |
| `weave_security_digest_size` | `digest` | `size_t` |
| `weave_security_digest` | `digest, data, size, out, out_size` | `bool` |
| `weave_security_hmac` | `digest, key, key_size, data, size, out, out_size` | `bool` |
| `weave_security_aes_cbc` | `encrypt, key, key_size, iv, input, size, output, output_size` | `bool` |
| `weave_security_aes_ecb` | `encrypt, key, key_size, input, output` | `bool` |
| `weave_security_ec_generate` | `curve, private_key, private_size, public_key, public_size` | `bool` |
| `weave_security_ec_public` | `curve, private_key, private_size, public_key, public_size` | `bool` |
| `weave_security_ecdsa_sign` | `curve, private_key, private_size, hash, hash_size, signature, signature_size` | `bool` |
| `weave_security_ecdsa_verify` | `curve, public_key, public_size, hash, hash_size, signature, signature_size` | `bool` |
| `weave_security_ecdh` | `curve, private_key, private_size, peer_key, peer_size, secret, secret_size` | `bool` |
| `weave_security_rsa_generate` | `bits, private_key, private_size, public_key, public_size` | `bool` |
| `weave_security_rsa_public` | `private_key, private_size, public_key, public_size` | `bool` |
| `weave_security_rsa_sign` | `pss, digest, private_key, private_size, hash, hash_size, signature, signature_size` | `bool` |
| `weave_security_rsa_verify` | `pss, digest, public_key, public_size, hash, hash_size, signature, signature_size` | `bool` |
| `weave_security_rsa_private_block` | `private_key, private_size, input, input_size, output, output_size` | `bool` |
| `weave_security_rsa_oaep_encrypt` | `public_key, public_size, label, label_size, input, input_size, output, output_size` | `bool` |
| `weave_security_rsa_oaep_decrypt` | `private_key, private_size, label, label_size, input, input_size, output, output_capacity, output_size` | `bool` |
| `weave_security_ed25519_generate` | `seed, seed_size, public_key, public_size` | `bool` |
| `weave_security_ed25519_public` | `seed, seed_size, public_key, public_size` | `bool` |
| `weave_security_ed25519_sign` | `seed, seed_size, message, message_size, signature, signature_size` | `bool` |
| `weave_security_ed25519_verify` | `public_key, public_size, message, message_size, signature, signature_size` | `bool` |
| `weave_security_ed25519ph_sign` | `seed, seed_size, digest, digest_size, signature, signature_size` | `bool` |
| `weave_security_ed25519ph_verify` | `public_key, public_size, digest, digest_size, signature, signature_size` | `bool` |
| `weave_secret_store` | `namespace_id, name, password, password_size, value, value_size` | `WeaveSecurityResult` |
| `weave_secret_load` | `namespace_id, name, password, password_size, output, capacity, output_size` | `WeaveSecurityResult` |
| `weave_secret_delete` | `namespace_id, name` | `WeaveSecurityResult` |
| `weave_permission_check` | `capability` | `bool` |
| `weave_permission_has` | `capability` | `bool` |
| `weave_permission_revoke_self` | — | `void` |

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
