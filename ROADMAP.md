# Roadmap

## [1.0.0]

- [ ] Validate macOS via GitHub Actions

## [1.0.1]

- [x] HTTPS headers and streamed uploads (API v3)
- [x] Bounded asynchronous FIFO and USB/BLE notifications
- [x] TCP/UDP/WebSocket event gateway across companions
- [x] Scoped socket operations and safe per-FAP cleanup
- [x] Companion opt-in, HTTPS validation and input bounds
- [x] Persistent startup service and native C regression tests

Scope: no IAM code, no insecure claim of FAP identity isolation or hardware-verified companion authentication. USB CDC/HID/CCID remains mutually exclusive; Weave may use BLE while a FAP owns USB. Full firmware-on-device validation is required before a production release.
