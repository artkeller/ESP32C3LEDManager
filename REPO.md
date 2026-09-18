# Repo-Struktur

Dieses Repository ist gleichzeitig eine **Arduino-Bibliothek** (erkennbar an
`library.properties` im Root) und, seit dem PlatformIO-Fix aus Issue #1, auch
direkt mit **PlatformIO** baubar (erkennbar an den `platformio.ini`-Dateien).
Beide Werkzeuge verstehen dasselbe `src/`-Layout, es gibt also keinen
Code-Fork zwischen den beiden Ökosystemen.

```
ESP32C3LEDManager/
├── library.properties          Arduino-Bibliotheks-Manifest (Name, Version,
│                                Abhängigkeiten, Kategorie). Wird sowohl vom
│                                Arduino Library Manager als auch von
│                                PlatformIOs Library-Dependency-Finder
│                                gelesen/respektiert.
├── platformio.ini               [NEU] Root-Projektkonfiguration für
│                                PlatformIO. "pio run" hier im Repo-Root
│                                baut standardmäßig examples/BasicDemo, mit
│                                zwei Environments core3/core2 (s.
│                                PLATFORMIO.md). Für Arduino-IDE-Nutzer ohne
│                                Bedeutung — ignoriert sie einfach.
├── .gitignore                   [NEU] Ignoriert PlatformIO-Build-Artefakte
│                                (.pio/, .pioenvs/, .piolibdeps/, …).
├── README.md                    Haupt-Dokumentation: Motivation, Hardware-
│                                Hintergrund (geteilter GPIO8), Installation,
│                                API-Übersicht.
├── PLATFORMIO.md                [NEU] Ausführlicher Hintergrund zu Issue #1:
│                                warum der LEDC-API-Wechsel in Arduino-ESP32
│                                3.0 PlatformIO-Nutzer betrifft, warum der im
│                                Issue vorgeschlagene Macro-Workaround nicht
│                                korrekt ist, und wie die Bibliothek das jetzt
│                                automatisch über beide Core-Generationen
│                                hinweg löst.
├── REPO.md                      [NEU] Diese Datei.
├── LICENSE                      Lizenztext.
├── CRA-EXEMPTION.md             Einordnung zum EU Cyber Resilience Act
│                                (reine Open-Source-Bibliothek).
│
├── src/                         Eigentlicher Bibliothekscode — von Arduino
│   │                            IDE und PlatformIO gleichermaßen als
│   │                            "src"-Ordner der Bibliothek erkannt.
│   ├── ESP32C3LEDManager.h      Klassendeklaration. [GEÄNDERT für Issue #1]
│   │                            Enthält jetzt die Compile-Zeit-Weiche
│   │                            ESP32C3LEDMANAGER_NEW_LEDC_API (auf Basis
│   │                            von esp_arduino_version.h) sowie die vier
│   │                            privaten LEDC-Wrapper-Methoden.
│   └── ESP32C3LEDManager.cpp    Implementierung. [GEÄNDERT für Issue #1]
│                                Alle direkten ledcAttach/ledcWrite/ledcRead/
│                                ledcDetach-Aufrufe laufen jetzt über die vier
│                                Wrapper, die je nach Core-Version die neue
│                                (pinbasierte) oder alte (kanalbasierte)
│                                LEDC-API ansprechen.
│
├── examples/
│   ├── BasicDemo/
│   │   ├── BasicDemo.ino         Einfaches Demo-Sketch (Arduino-IDE-Format,
│   │   │                          unverändert).
│   │   └── platformio.ini        [NEU] Eigenständiges PlatformIO-Projekt für
│   │                              genau dieses Beispiel; zieht die Bibliothek
│   │                              per "symlink://../.." aus dem Repo-Root.
│   │                              Environments core3 / core2, siehe
│   │                              PLATFORMIO.md.
│   └── FullDemo/
│       ├── FullDemo.ino          Umfangreicheres Demo-Sketch (Arduino-IDE-
│       │                          Format, unverändert).
│       └── platformio.ini        [NEU] Wie oben, für FullDemo.ino.
│
└── images/                      Timing-Diagramme und Board-Fotos, referenziert
                                  aus README.md (unverändert).
```

## Wie man das Repo nutzt

**Als Arduino-Bibliothek (unverändert):** ZIP herunterladen bzw. Repo klonen,
in der Arduino IDE über *Sketch → Include Library → Add .ZIP Library…*
einbinden (oder den Ordner direkt in `~/Arduino/libraries/` ablegen). Die
neuen PlatformIO-Dateien (`platformio.ini`, `PLATFORMIO.md`, `REPO.md`,
`.gitignore`) stören hier nicht — die Arduino IDE ignoriert sie.

**Als PlatformIO-Bibliothek/-Projekt (neu, Issue #1):** entweder

- `pio run -e core3` bzw. `-e core2` direkt im Repo-Root (baut `BasicDemo`),
  oder
- in `examples/BasicDemo/` bzw. `examples/FullDemo/` wechseln und dort
  `pio run -e core3` / `-e core2` ausführen, oder
- die Bibliothek als Abhängigkeit in einem eigenen PlatformIO-Projekt
  einbinden (`lib_deps = https://github.com/artkeller/ESP32C3LEDManager.git`).

Details zu den beiden Environments (`core3` = moderner Core via
pioarduino-Fork, `core2` = aktueller offizieller PlatformIO-Registry-Stand)
stehen in [PLATFORMIO.md](PLATFORMIO.md).
