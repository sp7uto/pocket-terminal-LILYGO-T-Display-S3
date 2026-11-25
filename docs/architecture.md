# Architecture / Architektura

Target: standard 1.9-inch non-touch T-Display-S3. Verify board revision before flashing.

Firmware layers: board/display, buttons, configuration, network, app registry, app modules. External sensors disabled by default. UI must distinguish unavailable, stale and valid data. Network operations must not block UI. No transmit or infrastructure control in initial release.

Installation: factory flash backup first; verified build artifact; Web Serial installer over HTTPS. OTA deferred until authenticated update and recovery are tested. Never publish credentials or device backups.

PL: warstwy sprzętowa, interfejs, konfiguracja, sieć i niezależne aplikacje. Brak zewnętrznych czujników na pierwszym etapie. Nieaktualne dane oznaczane jawnie. Instalacja dopiero po kopii Flash i sprawdzeniu kompilacji.
