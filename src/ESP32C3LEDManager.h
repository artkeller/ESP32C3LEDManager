#ifndef ESP32C3LEDMANAGER_H
#define ESP32C3LEDMANAGER_H

#include <Adafruit_NeoPixel.h>
#include <Arduino.h> // For uint8_t, uint16_t, etc.
#include <esp_arduino_version.h> // For ESP_ARDUINO_VERSION_VAL() -> LEDC API switch

// Arduino-ESP32 Core 3.0.0 (ESP-IDF 5.1) fundamentally reworked the LEDC API:
// old (Core < 3.0):  ledcSetup(channel, freq, res) + ledcAttachPin(pin, channel)
//                     ledcWrite(channel, duty) / ledcRead(channel) / ledcDetachPin(pin)
// new (Core >= 3.0): ledcAttach(pin, freq, res) — channel is assigned internally
//                     by the Peripheral Manager — ledcWrite(pin, duty) / ledcRead(pin) / ledcDetach(pin)
// See PLATFORMIO.md for the background (relevant in particular because the
// official PlatformIO Registry package "espressif32" is still stuck on Core 2.x).
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    #define ESP32C3LEDMANAGER_NEW_LEDC_API 1
#else
    #define ESP32C3LEDMANAGER_NEW_LEDC_API 0
#endif

class ESP32C3LEDManager {
public:
    // Constructor: initializes the manager for the shared GPIO pin and the number of NeoPixels.
    // sharedPin: the GPIO pin used by both LEDs (here GPIO8).
    // numPixels: the number of NeoPixel LEDs (here 1 for the onboard RGB LED).
    ESP32C3LEDManager(int sharedPin, int numPixels = 1);

    // --- Functions for controlling the blue LED ---
    // Turns the blue LED on (to the maximum safe brightness).
    void blueLEDOn();
    // Turns the blue LED off.
    void blueLEDOff();
    // Toggles the blue LED (between OFF and maximum safe brightness).
    void blueLEDToggle();
    // Fades the blue LED to a target brightness over a given duration.
    // targetBrightness: target brightness (0-240, to avoid triggering the NeoPixel).
    // durationMs: duration of the fade in milliseconds.
    void blueLEDFade(int targetBrightness, int durationMs);
    // Lets the blue LED blink as a square wave for a given duration.
    // frequencyHz: blink frequency in Hertz.
    // durationMs: total duration of the blinking in milliseconds.
    void blueLEDSquareWave(int frequencyHz, int durationMs);

    // --- Functions for controlling the NeoPixel LED ---
    // Sets the color of the NeoPixel LED using RGB values.
    // r, g, b: red, green, blue values (0-255).
    void neoPixelSetColor(uint8_t r, uint8_t g, uint8_t b);
    // Sets the color of the NeoPixel LED using HSV values.
    // hue: hue (0-65535).
    // sat: saturation (0-255).
    // val: brightness (0-255).
    void neoPixelSetColorHSV(uint16_t hue, uint8_t sat, uint8_t val);
    // Fades the NeoPixel LED through the color spectrum over a given duration.
    // durationMs: duration of the fade in milliseconds.
    void neoPixelColorFade(int durationMs);
    // Turns the NeoPixel LED to full white brightness.
    void neoPixelOn();
    // Turns the NeoPixel LED off.
    void neoPixelOff();
    // DEBUG INFO
    bool neoPixelIsInitialized() const;

private:
    int _sharedPin;
    int _numPixels;
    Adafruit_NeoPixel _strip; // The Adafruit NeoPixel object as a member
    bool _stripInitialized = false; // DEBUG INFO

    // LEDC parameters for the blue LED (tuned by the author)
    const int _blueLEDFreq = 1000;     // 1 kHz frequency
    const int _blueLEDResolution = 8;  // 8-bit resolution (values 0 to 255)
    // Maximum brightness for the blue LED, to avoid triggering the NeoPixel
    const int _blueLEDMaxBrightness = 240;
#if !ESP32C3LEDMANAGER_NEW_LEDC_API
    // Only needed for the old, channel-based LEDC API (Core < 3.0).
    // Fixed, since this manager controls exactly one PWM pin.
    static const int _blueLEDChannel = 0;
#endif

    // LEDC access is encapsulated behind these four helpers so the rest of
    // the class (blueLEDOn/Off/Fade/...) stays unchanged regardless of the
    // core version in use (Arduino IDE >=3.0 or, e.g., the PlatformIO
    // default <3.0). Implementation depends on ESP32C3LEDMANAGER_NEW_LEDC_API
    // and lives in ESP32C3LEDManager.cpp.
    void _ledcAttachBlue();
    void _ledcWriteBlue(uint32_t duty);
    uint32_t _ledcReadBlue();
    void _ledcDetachBlue();

    // Internal helper functions for managing the shared pin
    // Prepares the pin for NeoPixel operations (enables RMT, disables LEDC).
    void _activateNeoPixelMode();
    // Prepares the pin for blue LED (LEDC/PWM) operations (enables LEDC, disables RMT).
    void _activateBlueLEDMode();
    // Cleans up after blue LED (LEDC) operations and prepares the pin for NeoPixel again.
    void _deactivateBlueLEDMode();
};

#endif // ESP32C3LEDMANAGER_H
