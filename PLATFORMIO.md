# PlatformIO compatibility: background on Issue #1 ("compiler errors")

> **Honest disclaimer up front:** This library was developed and tested
> exclusively in the Arduino IDE so far. The fix described in this document
> was carefully derived from Espressif's official migration documentation
> and several verified GitHub issues, and it is internally consistent — but
> it has **not** been verified by an actual `pio run` in a reference
> environment ("flying blind"). Please have the issue reporter (or
> yourselves) build both environments from `platformio.ini` (`core3` and
> `core2`) once before tagging a release.

## 1. What was reported in the issue

[@spethwa reported](https://github.com/artkeller/ESP32C3LEDManager/issues/1)
that the library fails to compile under **PlatformIO**: `ledcAttach` and
`ledcDetach` were "apparently renamed in the framework". The workaround was:

```cpp
#define ledcDetach ledcDetachPin
#define ledcAttach ledcAttachPin
```

The compiler then complained about an "extra parameter" for
`ledcAttach(_sharedPin, _blueLEDFreq, _blueLEDResolution)`, so the third
parameter was simply commented out:

```cpp
ledcAttach(_sharedPin, _blueLEDFreq); //, _blueLEDResolution);
```

That silences the compiler — but, as explained below, it is **not a correct
fix**, just an accidentally-compiling patch over the symptom.

## 2. The actual root cause: the LEDC API overhaul in Arduino-ESP32 3.0

**Arduino-ESP32 Core 3.0.0** (summer 2024, based on ESP-IDF 5.1) fundamentally
reworked the LEDC (PWM) API. Espressif describes it like this in the official
[2.x → 3.0 migration guide](https://github.com/espressif/arduino-esp32/blob/master/docs/source/migration_guides/2.x_to_3.0.rst):

> The LEDC API has been changed in order to support the Peripheral Manager
> and make it easier to use, as LEDC channels are now automatically assigned
> to pins.

Specifically:

| | **Core < 3.0** (ESP-IDF 4.x, "old", *channel-based*) | **Core ≥ 3.0** (ESP-IDF 5.x, "new", *pin-based*) |
|---|---|---|
| Reserve + configure a channel | `ledcSetup(channel, freq, resolution)` | *not needed – handled automatically by the Peripheral Manager* |
| Attach to a pin | `ledcAttachPin(pin, channel)` | `ledcAttach(pin, freq, resolution)` *(merges both steps)* |
| Write PWM value | `ledcWrite(channel, duty)` | `ledcWrite(pin, duty)` |
| Read PWM value | `ledcRead(channel)` | `ledcRead(pin)` |
| Release a pin | `ledcDetachPin(pin)` | `ledcDetach(pin)` |

All addressing in the new API happens **through the pin**, not through a
manually managed channel number anymore. `ledcSetup` and `ledcAttachPin` were
removed entirely (not just renamed) — which is why a sketch built against the
old API fails to compile under Core ≥ 3.0 with
`'ledcSetup' was not declared in this scope`, and conversely a sketch using
the new API (like this library's original code) fails under Core < 3.0 with
`'ledcAttach' was not declared in this scope` — exactly the picture from
Issue #1.

## 3. Why this specifically shows up under PlatformIO

This library's original code already used the **new**, pin-based API
(`ledcAttach(_sharedPin, _blueLEDFreq, _blueLEDResolution)` etc.). That works
fine as long as an ESP32 board package ≥ 3.0.0 is installed in the Arduino
IDE — which is standard practice via the Boards Manager by now.

Under PlatformIO the situation differs: the **official**
[`platformio/platform-espressif32`](https://github.com/platformio/platform-espressif32)
package from the PlatformIO Registry (what you get with the default line
`platform = espressif32` in `platformio.ini`) has not been updated to an
Arduino-ESP32 core ≥ 3.0 for quite a while. According to the release notes,
its most recent version at the time of writing (6.10.0) still ships
**Arduino-ESP32 2.0.17**. Several GitHub discussions
([espressif/arduino-esp32#10039](https://github.com/espressif/arduino-esp32/discussions/10039),
[platformio/platform-espressif32#1225](https://github.com/platformio/platform-espressif32/issues/1225))
confirm, from both PlatformIO and Espressif maintainers, that official
PlatformIO Registry development for the Arduino-ESP32 core has effectively
stalled. The community fork
[`pioarduino/platform-espressif32`](https://github.com/pioarduino/platform-espressif32)
has established itself as the replacement, shipping current Arduino-ESP32
releases (3.x) as ready-to-use packages.

**In short:** anyone writing `platform = espressif32` (the default/official
registry) currently gets a core < 3.0 and therefore the *old* LEDC API —
exactly the combination under which @spethwa hit the error.

## 4. Why the quick fix from the issue doesn't actually work

The workaround

```cpp
#define ledcAttach ledcAttachPin
ledcAttach(_sharedPin, _blueLEDFreq); //, _blueLEDResolution);
```

expands to `ledcAttachPin(_sharedPin, _blueLEDFreq)`. But the old API's
signature is `ledcAttachPin(uint8_t pin, uint8_t channel)` — the second
parameter is a **channel number**, not a frequency! The code only compiles
because `_blueLEDFreq` (value `1000`) is, to the compiler, just a plain `int`
that gets silently truncated to `uint8_t channel` (with overflow:
`1000 % 256 = 232`). The ESP32-C3, however, only has **6 LEDC channels
(0–5)** — channel 232 simply doesn't exist. On top of that, without the
preceding `ledcSetup(channel, freq, resolution)` call, the PWM
frequency/resolution for that channel is never configured. So the blue LED's
fading and square-wave behavior with this workaround, if it worked at all,
would only have done so by accident or with the wrong frequency/resolution —
a classic "compiles, but doesn't do what you'd expect" case.

## 5. The fix in this repository: a version switch instead of a macro hack

Instead of redefining function names via `#define`, the library now checks
the actually-used core version at compile time
(`esp_arduino_version.h`, present in the Arduino-ESP32 core since 2.0.0) and
automatically selects the matching LEDC code path:

```cpp
// ESP32C3LEDManager.h
#include <esp_arduino_version.h>

#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    #define ESP32C3LEDMANAGER_NEW_LEDC_API 1
#else
    #define ESP32C3LEDMANAGER_NEW_LEDC_API 0
#endif
```

Four small private helper methods (`_ledcAttachBlue()`, `_ledcWriteBlue()`,
`_ledcReadBlue()`, `_ledcDetachBlue()`) encapsulate the actual LEDC calls;
the rest of the class's code (`blueLEDOn()`, `blueLEDFade()`,
`blueLEDSquareWave()`, etc.) is left unchanged and now only calls these
wrappers. In `ESP32C3LEDManager.cpp` each wrapper has two implementations —
one for `ESP32C3LEDMANAGER_NEW_LEDC_API == 1` (pin-based, see section 2,
right column) and one for `== 0` (channel-based, left column, with a fixed
`_blueLEDChannel = 0`).

**Result:** the same, unmodified source code compiles both with a modern
Arduino IDE core ≥ 3.0 and with the current PlatformIO default core 2.x —
with no macro redefinition and none of the parameter mix-up described in the
issue.

## 6. Building under PlatformIO

Both example sketches (`examples/BasicDemo`, `examples/FullDemo`) now ship
their own `platformio.ini` with two environments, plus an identically
structured `platformio.ini` in the repo root (builds `BasicDemo` by default):

```bash
# Option A: use a modern core via the pioarduino fork
pio run -e core3

# Option B: use the current PlatformIO Registry default (Core 2.x)
pio run -e core2
```

Both environments target `board = esp32-c3-devkitm-1` (a generic ESP32-C3
devkit definition — there is no dedicated board entry for the SuperMini /
SuperMini Plus; the generic definition matches electrically). If you'd like
to pin the core to ≥ 3.0 project-wide in your own `platformio.ini`, add:

```ini
platform = https://github.com/pioarduino/platform-espressif32/releases/download/51.03.03/platform-espressif32.zip
```

(this pioarduino release bundles Arduino-ESP32 3.0.3 / ESP-IDF 5.1 — the
version current at the time of writing this doc; check the
[pioarduino releases](https://github.com/pioarduino/platform-espressif32/releases)
for anything newer).

## 7. Known limitation / outlook

The fixed `_blueLEDChannel = 0` in the legacy path (Core < 3.0) is
unproblematic for this library as long as it is the **only** user of LEDC in
the sketch. Anyone who, under an old core, also uses LEDC channels themselves
(e.g. for a motor or a buzzer) and happens to occupy channel 0 as well will
hit a collision. A possible future enhancement would be to expose the
channel number as an optional constructor parameter (relevant only for
Core < 3.0) — deliberately not included in this bugfix PR, to keep the diff
small and focused on Issue #1.

## Sources

- Espressif, [Migration Guide 2.x → 3.0, LEDC section](https://github.com/espressif/arduino-esp32/blob/master/docs/source/migration_guides/2.x_to_3.0.rst)
- [espressif/arduino-esp32 Issue #9510](https://github.com/espressif/arduino-esp32/issues/9510) and [#10309](https://github.com/espressif/arduino-esp32/issues/10309) — examples of `'ledcSetup' was not declared` after upgrading to core 3.x (the reverse direction of the problem described here, confirming the same API boundary)
- [espressif/arduino-esp32 Discussion #10039](https://github.com/espressif/arduino-esp32/discussions/10039) and [platformio/platform-espressif32 Issue #1225](https://github.com/platformio/platform-espressif32/issues/1225) — the stall in official PlatformIO Registry support for Arduino-ESP32 ≥ 3.0
- [pioarduino/platform-espressif32](https://github.com/pioarduino/platform-espressif32) — community fork with current core releases

---
Written for **ESP32C3LEDManager v0.5.1**.
