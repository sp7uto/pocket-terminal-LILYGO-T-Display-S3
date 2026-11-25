# Diagnostic v0.0.1 — 2026-11-25

Firmware flashed successfully using Espressif Web Flasher. Factory flash backup verified before flashing.

| Test | Result |
|---|---|
| Readable display text | PASS |
| Landscape orientation | PASS |
| Flash 16 MiB | PASS |
| PSRAM 8 MiB at runtime | PASS |
| RGB and test colors | PASS |
| Button A navigation | PASS |
| Button B brightness adjustment | PASS |
| Wi-Fi scan | PASS |

All eight diagnostic tests passed on the development unit. Runtime PSRAM reports 8 MiB and physical backlight brightness changes correctly. Both buttons work. Hardware configuration accepted for v0.1.0. Do not change working display pin mapping without evidence. GitHub Pages installer returned HTTP 404 for firmware; manual browser flashing of the build artifact succeeded.
