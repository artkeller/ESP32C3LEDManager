# Repo structure

This repository is both an **Arduino library** (identifiable by
`library.properties` in the root) and, since the PlatformIO fix from
Issue #1, also directly buildable with **PlatformIO** (identifiable by the
`platformio.ini` files). Both toolchains understand the same `src/` layout,
so there is no code fork between the two ecosystems.

```
ESP32C3LEDManager/
├── library.properties          Arduino library manifest (name, version,
│                                dependencies, category). Read and respected
│                                by both the Arduino Library Manager and
│                                PlatformIO's Library Dependency Finder.
│                                Version bumped to 0.5.1 for this fix.
├── platformio.ini               [NEW] Root project configuration for
│                                PlatformIO. Running "pio run" here in the
│                                repo root builds examples/BasicDemo by
│                                default, with two environments core3/core2
│                                (see PLATFORMIO.md). Irrelevant to
│                                Arduino IDE users — simply ignored there.
├── .gitignore                   [NEW] Ignores PlatformIO build artifacts
│                                (.pio/, .pioenvs/, .piolibdeps/, …).
├── README.md                    Main documentation: motivation, hardware
│                                background (shared GPIO8), installation,
│                                API overview. Release badge bumped to
│                                v0.5.1.
├── PLATFORMIO.md                [NEW] Detailed background on Issue #1: why
│                                the LEDC API change in Arduino-ESP32 3.0
│                                affects PlatformIO users, why the macro
│                                workaround suggested in the issue is not
│                                correct, and how the library now resolves
│                                this automatically across both core
│                                generations.
├── REPO.md                      [NEW] This file.
├── LICENSE                      License text.
├── CRA-EXEMPTION.md             Note on the EU Cyber Resilience Act (pure
│                                open-source library).
│
├── src/                         The actual library code — recognized as
│   │                            the library's "src" folder by both the
│   │                            Arduino IDE and PlatformIO.
│   ├── ESP32C3LEDManager.h      Class declaration. [CHANGED for Issue #1]
│   │                            Now contains the compile-time switch
│   │                            ESP32C3LEDMANAGER_NEW_LEDC_API (based on
│   │                            esp_arduino_version.h) plus the four
│   │                            private LEDC wrapper methods. Comments
│   │                            translated to English; logic unchanged.
│   └── ESP32C3LEDManager.cpp    Implementation. [CHANGED for Issue #1]
│                                All direct ledcAttach/ledcWrite/ledcRead/
│                                ledcDetach calls now go through the four
│                                wrappers, which address the new (pin-based)
│                                or old (channel-based) LEDC API depending
│                                on the core version. Comments translated to
│                                English; logic unchanged.
│
├── examples/
│   ├── BasicDemo/
│   │   ├── BasicDemo.ino         Simple demo sketch (Arduino IDE format,
│   │   │                          unchanged).
│   │   └── platformio.ini        [NEW] Self-contained PlatformIO project
│   │                              for this specific example; pulls the
│   │                              library from the repo root via
│   │                              "symlink://../..". Environments core3 /
│   │                              core2, see PLATFORMIO.md.
│   └── FullDemo/
│       ├── FullDemo.ino          More extensive demo sketch (Arduino IDE
│       │                          format, unchanged).
│       └── platformio.ini        [NEW] Same as above, for FullDemo.ino.
│
└── images/                      Timing diagrams and board photos, referenced
                                  from README.md (unchanged).
```

## How to use this repo

**As an Arduino library (unchanged):** download the ZIP or clone the repo,
then import it in the Arduino IDE via *Sketch → Include Library → Add .ZIP
Library…* (or drop the folder directly into `~/Arduino/libraries/`). The new
PlatformIO files (`platformio.ini`, `PLATFORMIO.md`, `REPO.md`, `.gitignore`)
don't get in the way here — the Arduino IDE simply ignores them.

**As a PlatformIO library/project (new, Issue #1):** either

- run `pio run -e core3` or `-e core2` directly in the repo root (builds
  `BasicDemo`), or
- switch into `examples/BasicDemo/` or `examples/FullDemo/` and run
  `pio run -e core3` / `-e core2` there, or
- include the library as a dependency in your own PlatformIO project
  (`lib_deps = https://github.com/artkeller/ESP32C3LEDManager.git`).

Details on the two environments (`core3` = modern core via the pioarduino
fork, `core2` = current official PlatformIO Registry state) are in
[PLATFORMIO.md](PLATFORMIO.md).

---
Written for **ESP32C3LEDManager v0.5.1**.
