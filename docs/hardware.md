# Hardware / Sprzęt

## Verified hardware / Potwierdzone parametry

- ESP32-S3 QFN56, silicon revision v0.1
- Dual core, 240 MHz; 40 MHz crystal
- Embedded PSRAM: 8 MiB (AP_3v3)
- Flash: 16 MiB, JEDEC EF 4018, quad SPI, 3.3 V
- Native USB Serial/JTAG (VID 303A, PID 1001)
- Secure Boot and Flash Encryption disabled on development unit
- esptool 5.6.0

PCB revision, display controller and touch variant remain to be confirmed. No external sensors required for initial release.

## Backup / Kopia bezpieczeństwa

On 2026-11-25 two full 16 MiB flash reads from the development unit were verified to have identical SHA-256 hashes. Restore has not been tested. Device-specific identifiers, backup images and hashes are retained locally, not published.

Read-only backup command (replace COM6 as required):

```powershell
python -m esptool --chip esp32s3 --port COM6 --baud 460800 read-flash 0x0 0x1000000 factory-flash.bin
Get-FileHash factory-flash.bin -Algorithm SHA256
```

Repeat to a different file and compare both lengths and hashes before flashing. / Powtórz odczyt do osobnego pliku i porównaj rozmiary oraz sumy SHA-256 przed programowaniem.
