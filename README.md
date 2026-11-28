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
- Application-scoped software keystore and recoverable encrypted storage.
- SDK capability discovery.
- Native file, app and system management.
- Android-assisted firmware updates over USB/Bluetooth.
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
ufbt update --hw-target=f7 --local=build/Weave-1.0.4-sdk.zip
```

---

## 🧑‍💻 Developer API

SDK headers: [`<weave/weave.h>`](firmware/applications/services/weave/weave.h), [`<weave/weave_usb.h>`](firmware/applications/services/weave/weave_usb.h), [`<weave/weave_security.h>`](firmware/applications/services/weave/weave_security.h), [`<weave/weave_permissions.h>`](firmware/applications/services/weave/weave_permissions.h), [`<weave/weave_platform.h>`](firmware/applications/services/weave/weave_platform.h), [`<weave/weave_keystore.h>`](firmware/applications/services/weave/weave_keystore.h), [`<weave/weave_ble.h>`](firmware/applications/services/weave/weave_ble.h), [`<weave/weave_nfc.h>`](firmware/applications/services/weave/weave_nfc.h).

| API | Function | Arguments | Return |
|---|---|---|---|
| NFC ISO-DEP | `weave_nfc_acquire` | `owner` | `WeaveNfcResult` |
| NFC ISO-DEP | `weave_nfc_release` | `owner` | `WeaveNfcResult` |
| NFC ISO-DEP | `weave_nfc_receive` | `owner, request, data, capacity` | `WeaveNfcResult` |
| NFC ISO-DEP | `weave_nfc_respond` | `owner, id, data, size` | `WeaveNfcResult` |
| BLE GATT | `weave_ble_acquire` | `owner, config` | `WeaveBleResult` |
| BLE GATT | `weave_ble_release` | `owner` | `WeaveBleResult` |
| BLE GATT | `weave_ble_state` | `owner, state` | `WeaveBleResult` |
| BLE GATT | `weave_ble_receive` | `owner, connection, characteristic, data, capacity, size` | `WeaveBleResult` |
| BLE GATT | `weave_ble_update` | `owner, connection, characteristic, data, size` | `WeaveBleResult` |
| Networking | `weave_get_transport` | `weave` | `WeaveTransport` |
| Networking | `weave_transport_subscribe` | `weave, cb, context` | `bool` |
| Networking | `weave_transport_unsubscribe` | `weave, cb, context` | `void` |
| Networking | `weave_http_get` | `weave, url, cb, ctx` | `WeaveStatus` |
| Networking | `weave_http_request` | `weave, method, url, body, length, cb, ctx` | `WeaveStatus` |
| Networking | `weave_http_request_ex` | `weave, method, url, headers, header_count, body, length, cb, ctx` | `WeaveStatus` |
| Networking | `weave_http_upload_file_ex` | `weave, method, url, headers, header_count, sd_path, cb, ctx` | `WeaveStatus` |
| Networking | `weave_http_upload_file` | `weave, method, url, sd_path, cb, ctx` | `WeaveStatus` |
| Networking | `weave_socket_open` | `weave, type, host_or_wss_url, port, cb, ctx` | `WeaveStatus` |
| Networking | `weave_socket_send` | `weave, type, session, body, length, cb, ctx` | `WeaveStatus` |
| Networking | `weave_socket_receive` | `weave, type, session, cb, ctx` | `WeaveStatus` |
| Networking | `weave_socket_close` | `weave, type, session, cb, ctx` | `WeaveStatus` |
| Networking | `weave_socket_watch` | `weave, type, session, enabled, cb, ctx` | `WeaveStatus` |
| Networking | `weave_socket_subscribe_events` | `weave, cb, ctx` | `bool` |
| Networking | `weave_socket_unsubscribe_events` | `weave, cb, ctx` | `void` |
| Networking | `weave_context_release` | `weave, context, timeout_ms` | `bool` |
| USB | `weave_usb_acquire_ex` | `owner, mode, hid_report_descriptor, hid_descriptor_size, report_size` | `WeaveUsbResult` |
| USB | `weave_usb_acquire` | `owner, mode` | `WeaveUsbResult` |
| USB | `weave_usb_release` | `owner` | `WeaveUsbResult` |
| USB | `weave_usb_receive` | `owner, channel, buffer, capacity, size` | `WeaveUsbResult` |
| USB | `weave_usb_transmit` | `owner, channel, buffer, size` | `WeaveUsbResult` |
| USB | `weave_usb_ccid_notify` | `owner, present` | `WeaveUsbResult` |
| USB | `weave_usb_connected` | `owner` | `bool` |
| USB | `weave_usb_cdc_available` | — | `bool` |
| Security | `weave_security_random` | `out, size` | `bool` |
| Security | `weave_security_wipe` | `data, size` | `void` |
| Security | `weave_security_equal` | `a, b, size` | `bool` |
| Hashing / HMAC | `weave_security_sha256` | `data, size, out` | `bool` |
| Hashing / HMAC | `weave_security_hmac_sha256` | `key, key_len, data, size, out` | `bool` |
| Hashing / HMAC | `weave_security_hmac_sha256_init` | `ctx, key, key_size` | `bool` |
| Hashing / HMAC | `weave_security_hmac_sha256_compute` | `ctx, data, size, out` | `bool` |
| Hashing / HMAC | `weave_security_hmac_sha256_clear` | `ctx` | `void` |
| KDF | `weave_security_hkdf_sha256` | `key, key_size, salt, salt_size, info, info_size, out, out_size` | `bool` |
| KDF | `weave_security_pbkdf2_sha256` | `password, password_size, salt, salt_size, iterations, out, out_size` | `bool` |
| Security | `weave_security_api_version` | — | `uint32_t` |
| Crypto Jobs | `weave_security_job_begin_pbkdf2` | `job, password, password_size, salt, salt_size, iterations, out_size` | `bool` |
| Crypto Jobs | `weave_security_job_begin_sha256` | `job, data, size` | `bool` |
| Crypto Jobs | `weave_security_job_begin_hmac_sha256` | `job, key, key_size, data, size` | `bool` |
| Crypto Jobs | `weave_security_job_step` | `job, budget` | `WeaveSecurityJobState` |
| Crypto Jobs | `weave_security_job_progress` | `job, done, total` | `bool` |
| Crypto Jobs | `weave_security_job_result` | `job, out, size` | `bool` |
| Crypto Jobs | `weave_security_job_cancel` | `job` | `void` |
| Encryption | `weave_security_aead_encrypt` | `key, nonce, aad, aad_size, plaintext, size, ciphertext, tag` | `bool` |
| Encryption | `weave_security_aead_decrypt` | `key, nonce, aad, aad_size, ciphertext, size, tag, plaintext` | `bool` |
| Encryption | `weave_security_aead_hw_selftest` | — | `bool` |
| Encryption | `weave_security_aead_encrypt_hw` | `key, nonce, aad, aad_size, plaintext, size, ciphertext, tag` | `bool` |
| Encryption | `weave_security_aead_decrypt_hw` | `key, nonce, aad, aad_size, ciphertext, size, tag, plaintext` | `bool` |
| Hashing / HMAC | `weave_security_digest_size` | `digest` | `size_t` |
| Hashing / HMAC | `weave_security_digest` | `digest, data, size, out, out_size` | `bool` |
| Hashing / HMAC | `weave_security_hmac` | `digest, key, key_size, data, size, out, out_size` | `bool` |
| Encryption | `weave_security_aes_cbc` | `encrypt, key, key_size, iv, input, size, output, output_size` | `bool` |
| Encryption | `weave_security_aes_ecb` | `encrypt, key, key_size, input, output` | `bool` |
| Asymmetric | `weave_security_ec_generate` | `curve, private_key, private_size, public_key, public_size` | `bool` |
| Asymmetric | `weave_security_ec_public` | `curve, private_key, private_size, public_key, public_size` | `bool` |
| Asymmetric | `weave_security_ecdsa_sign` | `curve, private_key, private_size, hash, hash_size, signature, signature_size` | `bool` |
| Asymmetric | `weave_security_ecdsa_verify` | `curve, public_key, public_size, hash, hash_size, signature, signature_size` | `bool` |
| Asymmetric | `weave_security_ecdh` | `curve, private_key, private_size, peer_key, peer_size, secret, secret_size` | `bool` |
| Asymmetric | `weave_security_rsa_generate` | `bits, private_key, private_size, public_key, public_size` | `bool` |
| Asymmetric | `weave_security_rsa_public` | `private_key, private_size, public_key, public_size` | `bool` |
| Asymmetric | `weave_security_rsa_sign` | `pss, digest, private_key, private_size, hash, hash_size, signature, signature_size` | `bool` |
| Asymmetric | `weave_security_rsa_verify` | `pss, digest, public_key, public_size, hash, hash_size, signature, signature_size` | `bool` |
| Asymmetric | `weave_security_rsa_private_block` | `private_key, private_size, input, input_size, output, output_size` | `bool` |
| Asymmetric | `weave_security_rsa_oaep_encrypt` | `public_key, public_size, label, label_size, input, input_size, output, output_size` | `bool` |
| Asymmetric | `weave_security_rsa_oaep_decrypt` | `private_key, private_size, label, label_size, input, input_size, output, output_capacity, output_size` | `bool` |
| Asymmetric | `weave_security_ed25519_generate` | `seed, seed_size, public_key, public_size` | `bool` |
| Asymmetric | `weave_security_ed25519_public` | `seed, seed_size, public_key, public_size` | `bool` |
| Asymmetric | `weave_security_ed25519_sign` | `seed, seed_size, message, message_size, signature, signature_size` | `bool` |
| Asymmetric | `weave_security_ed25519_verify` | `public_key, public_size, message, message_size, signature, signature_size` | `bool` |
| Asymmetric | `weave_security_ed25519ph_sign` | `seed, seed_size, digest, digest_size, signature, signature_size` | `bool` |
| Asymmetric | `weave_security_ed25519ph_verify` | `public_key, public_size, digest, digest_size, signature, signature_size` | `bool` |
| Secure Storage | `weave_secret_store` | `namespace_id, name, password, password_size, value, value_size` | `WeaveSecurityResult` |
| Secure Storage | `weave_secret_load` | `namespace_id, name, password, password_size, output, capacity, output_size` | `WeaveSecurityResult` |
| Secure Storage | `weave_secret_delete` | `namespace_id, name` | `WeaveSecurityResult` |
| Permissions | `weave_permission_check` | `capability` | `bool` |
| Permissions | `weave_permission_has` | `capability` | `bool` |
| Permissions | `weave_permission_revoke_self` | — | `void` |
| Platform | `weave_platform_capabilities` | — | `uint32_t` |
| Platform | `weave_platform_state` | `weave` | `uint32_t` |
| Keystore | `weave_keystore_generate` | `ns, name, algorithm, password, password_size, public_key, public_size` | `WeaveSecurityResult` |
| Keystore | `weave_keystore_public` | `ns, name, algorithm, password, password_size, public_key, public_size` | `WeaveSecurityResult` |
| Keystore | `weave_keystore_sign` | `ns, name, algorithm, password, password_size, pss, digest, input, input_size, signature, signature_size` | `WeaveSecurityResult` |
| Keystore | `weave_keystore_derive` | `ns, name, algorithm, password, password_size, peer, peer_size, secret, secret_size` | `WeaveSecurityResult` |
| Keystore | `weave_keystore_decrypt_oaep` | `ns, name, algorithm, password, password_size, label, label_size, input, input_size, output, capacity, output_size` | `WeaveSecurityResult` |
| Keystore | `weave_keystore_delete` | `ns, name` | `WeaveSecurityResult` |

### Example

```c
#include <furi.h>
#include <weave/weave_security.h>

int32_t example_app(void* context) {
    UNUSED(context);
    uint8_t hash[32];
    if(weave_security_sha256((const uint8_t*)"Hello Weave", 11, hash))
        FURI_LOG_I("Example", "SHA-256 computed successfully");
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
