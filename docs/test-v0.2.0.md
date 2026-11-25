# Pocket Terminal v0.2.0 — METEO acceptance test

Date: 2026-11-25

All ten tests reported PASS by the hardware owner:

- Wi-Fi settings preserved during browser app-only update.
- Temperature displayed in HOME.
- Temperature, humidity and pressure displayed in METEO.
- Cloud cover displayed.
- Wind speed and direction displayed.
- Daily minimum and maximum temperatures displayed.
- Sunrise and sunset displayed.
- Button A changes METEO pages.
- Button B refreshes weather data.
- LIVE status visible.

Result: 10/10 PASS. Keep the tested application-only browser flashing mechanism (eraseAll: false) for compatible upgrades. External sensors remain deferred.

PL: Wszystkie dziesięć testów zaliczono. Aktualizacja przez przeglądarkę zachowała ustawienia Wi-Fi. Moduł METEO zaakceptowany. Następny etap: NETWORK v0.3.0.
