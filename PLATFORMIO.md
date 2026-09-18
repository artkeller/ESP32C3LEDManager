# PlatformIO-Kompatibilität: Hintergrund zu Issue #1 ("compiler errors")

> **Ehrlicher Hinweis vorweg:** Diese Bibliothek wurde bisher ausschließlich in
> der Arduino IDE entwickelt und getestet. Die in diesem Dokument beschriebene
> Lösung ist auf Basis der offiziellen Migrationsdokumentation von
> Arduino-ESP32 sowie mehrerer verifizierter GitHub-Issues sorgfältig
> hergeleitet und in sich logisch konsistent — sie wurde aber **nicht** durch
> einen echten `pio run` in einer Referenzumgebung gegenteilig verifiziert
> ("Blindflug"). Bitte den Issue-Reporter (oder euch selbst) bitten, die
> beiden Environments aus `platformio.ini` (`core3` und `core2`) einmal
> gegenzutesten, bevor ein Release getaggt wird.

## 1. Was im Issue gemeldet wurde

[@spethwa meldet](https://github.com/artkeller/ESP32C3LEDManager/issues/1),
dass die Bibliothek unter **PlatformIO** nicht kompiliert: `ledcAttach` und
`ledcDetach` seien "apparently renamed in the framework". Der Workaround war:

```cpp
#define ledcDetach ledcDetachPin
#define ledcAttach ledcAttachPin
```

Danach beschwerte sich der Compiler über einen "extra parameter" bei
`ledcAttach(_sharedPin, _blueLEDFreq, _blueLEDResolution)`, woraufhin der
dritte Parameter einfach auskommentiert wurde:

```cpp
ledcAttach(_sharedPin, _blueLEDFreq); //, _blueLEDResolution);
```

Das bringt den Compiler zum Schweigen — ist aber, wie unten erklärt, **keine
korrekte Lösung**, sondern nur ein zufällig kompilierendes Symptom-Pflaster.

## 2. Die eigentliche Ursache: der LEDC-API-Umbau in Arduino-ESP32 3.0

Mit **Arduino-ESP32 Core 3.0.0** (Sommer 2024, basiert auf ESP-IDF 5.1) wurde
die LEDC-(PWM-)API grundlegend umgebaut. Espressif hat das in der offiziellen
[Migrationsanleitung 2.x → 3.0](https://github.com/espressif/arduino-esp32/blob/master/docs/source/migration_guides/2.x_to_3.0.rst)
so beschrieben:

> Die LEDC-API wurde geändert, um den Peripheral Manager zu unterstützen und
> die Nutzung zu vereinfachen, da LEDC-Kanäle jetzt automatisch den Pins
> zugewiesen werden.

Konkret:

| | **Core < 3.0** (ESP-IDF 4.x, „alt", *kanalbasiert*) | **Core ≥ 3.0** (ESP-IDF 5.x, „neu", *pinbasiert*) |
|---|---|---|
| Kanal reservieren + konfigurieren | `ledcSetup(channel, freq, resolution)` | *entfällt – automatisch vom Peripheral Manager* |
| An Pin anhängen | `ledcAttachPin(pin, channel)` | `ledcAttach(pin, freq, resolution)` *(fasst beides zusammen)* |
| PWM-Wert schreiben | `ledcWrite(channel, duty)` | `ledcWrite(pin, duty)` |
| PWM-Wert lesen | `ledcRead(channel)` | `ledcRead(pin)` |
| Pin freigeben | `ledcDetachPin(pin)` | `ledcDetach(pin)` |

Alle Adressierung erfolgt in der neuen API also **über den Pin**, nicht mehr
über eine selbst verwaltete Kanalnummer. `ledcSetup` und `ledcAttachPin`
wurden komplett entfernt (nicht nur umbenannt) — deshalb schlägt ein Sketch,
der auf der alten API basiert, unter Core ≥ 3.0 mit
`'ledcSetup' was not declared in this scope` fehl, und umgekehrt schlägt ein
Sketch mit der neuen API (wie der ursprüngliche Code dieser Bibliothek) unter
Core < 3.0 mit `'ledcAttach' was not declared in this scope` fehl — exakt das
Bild aus Issue #1.

## 3. Warum das speziell unter PlatformIO auffällt

Der ursprüngliche Code dieser Bibliothek verwendet bereits die **neue**,
pinbasierte API (`ledcAttach(_sharedPin, _blueLEDFreq, _blueLEDResolution)`
usw.). Das funktioniert problemlos, **solange** in der Arduino IDE ein
ESP32-Board-Paket ≥ 3.0.0 installiert ist — was über den Boards-Manager
mittlerweile Standard ist.

Unter PlatformIO ist die Lage anders: Das **offizielle**
[`platformio/platform-espressif32`](https://github.com/platformio/platform-espressif32)-Package
aus der PlatformIO-Registry (das, was man mit der Standardzeile
`platform = espressif32` in `platformio.ini` bekommt) wurde seit längerem
nicht mehr auf einen Arduino-ESP32-Core ≥ 3.0 aktualisiert. Laut den
Release-Notes lag es zuletzt (Version 6.10.0) noch bei **Arduino-ESP32
2.0.17** (ESP-IDF 5.3 für ESP-IDF selbst, aber Arduino-Core-seitig weiterhin
die 2.x-API-Generation). In mehreren GitHub-Diskussionen
([espressif/arduino-esp32#10039](https://github.com/espressif/arduino-esp32/discussions/10039),
[platformio/platform-espressif32#1225](https://github.com/platformio/platform-espressif32/issues/1225))
bestätigen sowohl PlatformIO- als auch Espressif-Maintainer, dass die
offizielle PlatformIO-Registry-Entwicklung für den Arduino-ESP32-Kern faktisch
zum Stillstand gekommen ist. Als Ersatz hat sich der Community-Fork
[`pioarduino/platform-espressif32`](https://github.com/pioarduino/platform-espressif32)
etabliert, der aktuelle Arduino-ESP32-Releases (3.x) als fertige Pakete
bereitstellt.

**Kurz:** Wer `platform = espressif32` (Standard/offizielle Registry) schreibt,
bekommt aktuell einen Core < 3.0 und damit die *alte* LEDC-API — genau die
Kombination, unter der @spethwa den Fehler bekommen hat.

## 4. Warum der Quick-Fix aus dem Issue nicht wirklich funktioniert

Der Workaround

```cpp
#define ledcAttach ledcAttachPin
ledcAttach(_sharedPin, _blueLEDFreq); //, _blueLEDResolution);
```

expandiert zu `ledcAttachPin(_sharedPin, _blueLEDFreq)`. Die Signatur von
`ledcAttachPin` in der alten API lautet aber `ledcAttachPin(uint8_t pin,
uint8_t channel)` — der zweite Parameter ist eine **Kanalnummer**, keine
Frequenz! Der Code kompiliert nur deshalb, weil `_blueLEDFreq` (Wert `1000`)
für den Compiler ein ganz normaler `int` ist, der klaglos auf `uint8_t
channel` gecastet wird (mit Overflow: `1000 % 256 = 232`). Der ESP32-C3
besitzt aber nur **6 LEDC-Kanäle (0–5)** — Kanal 232 existiert schlicht
nicht. Hinzu kommt: Ohne den vorherigen `ledcSetup(channel, freq,
resolution)`-Aufruf wird die PWM-Frequenz/-Auflösung für diesen Kanal nie
konfiguriert. Das Blau-LED-Fading und die Rechteckwelle liefen mit diesem
Workaround also, wenn überhaupt, nur zufällig bzw. mit falscher
Frequenz/Auflösung — ein klassischer "kompiliert, tut aber nicht das
Erwartete"-Fall.

## 5. Die Lösung in diesem Repository: Versionsweiche statt Macro-Hack

Statt Funktionsnamen per `#define` umzubiegen, prüft die Bibliothek jetzt zur
Compile-Zeit die tatsächlich verwendete Core-Version
(`esp_arduino_version.h`, im Arduino-ESP32-Core seit 2.0.0 vorhanden) und
wählt automatisch den passenden LEDC-Code-Pfad:

```cpp
// ESP32C3LEDManager.h
#include <esp_arduino_version.h>

#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    #define ESP32C3LEDMANAGER_NEW_LEDC_API 1
#else
    #define ESP32C3LEDMANAGER_NEW_LEDC_API 0
#endif
```

Vier kleine private Hilfsmethoden (`_ledcAttachBlue()`, `_ledcWriteBlue()`,
`_ledcReadBlue()`, `_ledcDetachBlue()`) kapseln die eigentlichen LEDC-Aufrufe;
der komplette restliche Code der Klasse (`blueLEDOn()`, `blueLEDFade()`,
`blueLEDSquareWave()` usw.) bleibt unverändert und ruft nur noch diese
Wrapper auf. In `ESP32C3LEDManager.cpp` gibt es je Wrapper zwei Implementierungen
— eine für `ESP32C3LEDMANAGER_NEW_LEDC_API == 1` (pinbasiert, s. Abschnitt 2,
rechte Spalte) und eine für `== 0` (kanalbasiert, linke Spalte, mit fest
reserviertem `_blueLEDChannel = 0`).

**Ergebnis:** Derselbe, unveränderte Quellcode kompiliert sowohl mit einem
modernen Arduino-IDE-Core ≥ 3.0 als auch mit dem aktuellen
PlatformIO-Standard-Core 2.x — ganz ohne Macro-Umdefinition und ohne die im
Issue beschriebene Parameterverwechslung.

## 6. Bauen unter PlatformIO

Für beide Beispiel-Sketches (`examples/BasicDemo`, `examples/FullDemo`) liegt
jetzt eine eigene `platformio.ini` mit zwei Environments bei, plus eine
identisch aufgebaute `platformio.ini` im Repo-Root (baut standardmäßig
`BasicDemo`):

```bash
# Variante A: modernen Core über den pioarduino-Fork verwenden
pio run -e core3

# Variante B: aktuellen PlatformIO-Registry-Standard (Core 2.x) verwenden
pio run -e core2
```

Beide Environments zielen auf `board = esp32-c3-devkitm-1` (generische
ESP32-C3-DevKit-Definition — für das SuperMini/SuperMini Plus gibt es keinen
eigenen Board-Eintrag; elektrisch passt die generische Definition). Wer den
Core dauerhaft/projektweit auf ≥ 3.0 fixieren möchte, trägt in der eigenen
`platformio.ini`

```ini
platform = https://github.com/pioarduino/platform-espressif32/releases/download/51.03.03/platform-espressif32.zip
```

ein (dieses pioarduino-Release bündelt Arduino-ESP32 3.0.3 /
ESP-IDF 5.1 — Versionsstand zum Zeitpunkt dieser Doku; für eine neuere
Version bitte die [pioarduino-Releases](https://github.com/pioarduino/platform-espressif32/releases)
prüfen).

## 7. Bekannte Einschränkung / Ausblick

Der fest verdrahtete `_blueLEDChannel = 0` im Legacy-Pfad (Core < 3.0) ist für
diese Bibliothek unproblematisch, solange sie der **einzige** Nutzer von LEDC
im Sketch ist. Wer unter einem alten Core zusätzlich selbst LEDC-Kanäle
verwendet (z. B. für einen Motor oder Buzzer) und dabei ebenfalls Kanal 0
belegt, bekommt eine Kollision. Eine mögliche spätere Erweiterung wäre, die
Kanalnummer als optionalen Konstruktor-Parameter (nur relevant für Core < 3.0)
freizugeben — für diesen Bugfix-PR aber bewusst nicht mit umgesetzt, um den
Diff klein und fokussiert auf Issue #1 zu halten.

## Quellen

- Espressif, [Migration Guide 2.x → 3.0, Abschnitt LEDC](https://github.com/espressif/arduino-esp32/blob/master/docs/source/migration_guides/2.x_to_3.0.rst)
- [espressif/arduino-esp32 Issue #9510](https://github.com/espressif/arduino-esp32/issues/9510) und [#10309](https://github.com/espressif/arduino-esp32/issues/10309) — Beispiele für `'ledcSetup' was not declared` nach Core-Upgrade auf 3.x (die Gegenrichtung des hier beschriebenen Problems, bestätigt dieselbe API-Grenze)
- [espressif/arduino-esp32 Discussion #10039](https://github.com/espressif/arduino-esp32/discussions/10039) und [platformio/platform-espressif32 Issue #1225](https://github.com/platformio/platform-espressif32/issues/1225) — Stillstand der offiziellen PlatformIO-Registry-Unterstützung für Arduino-ESP32 ≥ 3.0
- [pioarduino/platform-espressif32](https://github.com/pioarduino/platform-espressif32) — Community-Fork mit aktuellen Core-Releases
