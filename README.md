# Pocket Terminal – LILYGO T-Display-S3

[Polski](#polski) | [English](#english)

**Status:** pre-alpha / w przygotowaniu. No tested firmware yet.

## Polski

Uniwersalny terminal kieszonkowy dla standardowego LILYGO T-Display-S3 1,9 cala bez dotyku.

| Moduł | Funkcja |
|---|---|
| HOME | Zegar, Wi-Fi, akumulator |
| RADIO | Odczyt CAT przez bramkę sieciową |
| APRS | Radar lokalny i śledzenie stacji |
| SIEĆ | Monitoring infrastruktury |
| HA | Home Assistant |
| METEO | Dane pogodowe |
| MQTT | Podgląd komunikatów |
| CZUJNIKI | Opcjonalny, odłożony |

Plan: PlatformIO, GitHub Actions, instalator Web Serial, konfiguracja WWW i zabezpieczone OTA. Przed instalacją wykonać kopię fabrycznej pamięci Flash. Domyślnie tylko odczyt, bez PTT i sterowania krytyczną infrastrukturą.

## English

Modular pocket terminal for the standard non-touch 1.9-inch LILYGO T-Display-S3. Planned modules: HOME, RADIO, APRS, NETWORK, HOME ASSISTANT, WEATHER, MQTT and optional SENSORS (deferred).

Roadmap: PlatformIO, GitHub Actions, Web Serial installer, web configuration and secured OTA. Back up factory flash before installation. Read-only by default; no PTT or critical infrastructure control.

Hardware reference: https://github.com/Xinyuan-LilyGO/T-Display-S3
