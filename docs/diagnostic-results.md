# Diagnostic v0.0.1 — 2026-11-25

Firmware flashed successfully using Espressif Web Flasher. Factory flash backup verified before flashing.

| Test | Result |
|---|---|
| Readable display text | PASS |
| Landscape orientation | PASS |
| Flash 16 MiB | PASS |
| PSRAM 8 MiB at runtime | Unconfirmed |
| RGB and test colors | PASS |
| Button A navigation | PASS |
| Button B brightness adjustment | Unconfirmed |
| Wi-Fi scan | PASS |

Physical PSRAM was detected by esptool prior to flashing. Button B operation is indirectly supported by successful Wi-Fi scan, but brightness adjustment remains unverified. Do not change working display pin mapping without evidence. GitHub Pages installer returned HTTP 404 for firmware; manual browser flashing of the build artifact succeeded.
