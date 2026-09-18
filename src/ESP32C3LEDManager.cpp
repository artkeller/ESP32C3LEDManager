#include "ESP32C3LEDManager.h"

// Constructor
ESP32C3LEDManager::ESP32C3LEDManager(int sharedPin, int numPixels)
    : _sharedPin(sharedPin), _numPixels(numPixels),
      _strip(numPixels, sharedPin, NEO_GRB + NEO_KHZ800) {
    // Initial setup in the constructor
    // Make sure the shared pin is initially configured as a digital output and is OFF.
    // The blue LED is NOT inverted: LOW = OFF, HIGH = ON.
    pinMode(_sharedPin, OUTPUT);  
    digitalWrite(_sharedPin, LOW);

    // Initialize NeoPixel (configures RMT for the shared pin).
    _stripInitialized  = _strip.begin();  // begin() returns a bool
    if(_stripInitialized ) {
        _strip.clear();     // Set all pixels to 0 (optional, but cleaner)
        _strip.show();      // Send signal to the LEDs (all OFF)
    }
}

bool ESP32C3LEDManager::neoPixelIsInitialized() const {
    return _stripInitialized;
}

// --- LEDC compatibility layer (Core >= 3.0 vs. < 3.0) ---
// See PLATFORMIO.md for the detailed explanation of why this switch is
// necessary (in particular for PlatformIO default environments with Core < 3.0).
#if ESP32C3LEDMANAGER_NEW_LEDC_API
// Core >= 3.0 (ESP-IDF 5.1+): pin-based API, channel is managed automatically.
void ESP32C3LEDManager::_ledcAttachBlue() {
    ledcAttach(_sharedPin, _blueLEDFreq, _blueLEDResolution);
}
void ESP32C3LEDManager::_ledcWriteBlue(uint32_t duty) {
    ledcWrite(_sharedPin, duty);
}
uint32_t ESP32C3LEDManager::_ledcReadBlue() {
    return ledcRead(_sharedPin);
}
void ESP32C3LEDManager::_ledcDetachBlue() {
    ledcDetach(_sharedPin);
}
#else
// Core < 3.0 (ESP-IDF 4.4, e.g. the official PlatformIO Registry package):
// classic, channel-based API — the channel must be reserved explicitly.
void ESP32C3LEDManager::_ledcAttachBlue() {
    ledcSetup(_blueLEDChannel, _blueLEDFreq, _blueLEDResolution);
    ledcAttachPin(_sharedPin, _blueLEDChannel);
}
void ESP32C3LEDManager::_ledcWriteBlue(uint32_t duty) {
    ledcWrite(_blueLEDChannel, duty);
}
uint32_t ESP32C3LEDManager::_ledcReadBlue() {
    return ledcRead(_blueLEDChannel);
}
void ESP32C3LEDManager::_ledcDetachBlue() {
    ledcDetachPin(_sharedPin);
}
#endif

// Internal helper function: prepares the pin for NeoPixel operations
void ESP32C3LEDManager::_activateNeoPixelMode() {
    // Detach the LEDC peripheral from the pin, if active.
    _ledcDetachBlue();
    // Make sure the pin is configured as a digital output and is LOW.
    // This is essential to prevent the NeoPixel LED from glitching to white.
    pinMode(_sharedPin, OUTPUT);
    digitalWrite(_sharedPin, LOW);
    delay(50); // Short delay to let the pin state settle.

    // Re-initialize the NeoPixel library (re-binds RMT to the pin).
    _strip.begin();
}

// Internal helper function: prepares the pin for blue LED (LEDC) operations
void ESP32C3LEDManager::_activateBlueLEDMode() {
    // Turn off the NeoPixel and free the RMT resources (as far as possible).
    _strip.clear();
    _strip.show();
    // Make sure the pin is digitally LOW before attaching LEDC, to avoid glitches.
    pinMode(_sharedPin, OUTPUT);
    digitalWrite(_sharedPin, LOW);
    delay(50); // Short delay to let the pin state settle.

    // Attach the LEDC peripheral to the pin (configures the pin for PWM).
    _ledcAttachBlue();
}

// Internal helper function: cleans up after blue LED (LEDC) operations and prepares for NeoPixel
void ESP32C3LEDManager::_deactivateBlueLEDMode() {
    // Make sure the blue LED is turned off via LEDC.
    _ledcWriteBlue(0); // 0 = OFF for the non-inverted blue LED.
    // Detach the LEDC peripheral from the pin.
    _ledcDetachBlue();

    // Immediately prepare the pin for NeoPixel operations again and turn the NeoPixel off.
    _activateNeoPixelMode();
    _strip.clear();
    _strip.show();
}

// --- Implementation of the functions for controlling the blue LED ---
void ESP32C3LEDManager::blueLEDOn() {
    _activateBlueLEDMode();
    _ledcWriteBlue(_blueLEDMaxBrightness); // Turn on at maximum safe brightness.
}

void ESP32C3LEDManager::blueLEDOff() {
    _activateBlueLEDMode(); // Attach LEDC so we can turn off via PWM.
    _ledcWriteBlue(0); // Turn off.
    _deactivateBlueLEDMode(); // Detach LEDC and prepare for NeoPixel.
}

void ESP32C3LEDManager::blueLEDToggle() {
    _activateBlueLEDMode();
    // Read the current duty cycle. If 0 (off), turn on. Otherwise turn off.
    if (_ledcReadBlue() == 0) {
        _ledcWriteBlue(_blueLEDMaxBrightness);
    } else {
        _ledcWriteBlue(0);
        _deactivateBlueLEDMode(); // Clean up when turning off.
    }
}

void ESP32C3LEDManager::blueLEDFade(int targetBrightness, int durationMs) {
    _activateBlueLEDMode();
    int currentBrightness = _ledcReadBlue(); // Fetch the current brightness.
    // Make sure the target brightness stays within the safe limits.
    if (targetBrightness > _blueLEDMaxBrightness) targetBrightness = _blueLEDMaxBrightness;
    if (targetBrightness < 0) targetBrightness = 0;

    int steps = abs(targetBrightness - currentBrightness);
    if (steps == 0) {
        if (targetBrightness == 0) _deactivateBlueLEDMode(); // Clean up if already off.
        return;
    }
    int delayPerStep = durationMs / steps;
    if (delayPerStep == 0) delayPerStep = 1; // Minimum delay.

    if (targetBrightness > currentBrightness) {
        for (int brightness = currentBrightness; brightness <= targetBrightness; brightness++) {
            _ledcWriteBlue(brightness);
            delay(delayPerStep);
        }
    } else {
        for (int brightness = currentBrightness; brightness >= targetBrightness; brightness--) {
            _ledcWriteBlue(brightness);
            delay(delayPerStep);
        }
    }
    // Clean up if fading to OFF. Otherwise keep LEDC attached.
    if (targetBrightness == 0) {
        _deactivateBlueLEDMode();
    }
}

void ESP32C3LEDManager::blueLEDSquareWave(int frequencyHz, int durationMs) {
    _activateBlueLEDMode();
    long startTime = millis();
    long endTime = startTime + durationMs;
    unsigned long halfPeriodMs = 1000 / (2 * frequencyHz); // Half period for ON/OFF.

    while (millis() < endTime) {
        _ledcWriteBlue(_blueLEDMaxBrightness); // ON
        delay(halfPeriodMs);
        if (millis() >= endTime) break; // Check whether the duration expired during the ON state.
        _ledcWriteBlue(0); // OFF
        delay(halfPeriodMs);
    }
    _deactivateBlueLEDMode(); // Make sure the LED is off and clean up.
}

// --- Implementation of the functions for controlling the NeoPixel LED ---
void ESP32C3LEDManager::neoPixelSetColor(uint8_t r, uint8_t g, uint8_t b) {
    _activateNeoPixelMode();
    _strip.setPixelColor(0, r, g, b);
    _strip.show();
}

void ESP32C3LEDManager::neoPixelSetColorHSV(uint16_t hue, uint8_t sat, uint8_t val) {
    _activateNeoPixelMode();
    _strip.setPixelColor(0, _strip.ColorHSV(hue, sat, val));
    _strip.show();
}

void ESP32C3LEDManager::neoPixelColorFade(int durationMs) {
    _activateNeoPixelMode();
    long startTime = millis();
    long endTime = startTime + durationMs;
    // Adjust the hue step to cover the entire spectrum.
    int hueStep = 65536 / (durationMs / 10);
    if (hueStep == 0) hueStep = 1; // Ensure at least 1 step.

    for (int hue = 0; hue < 65536; hue += hueStep) {
        if (millis() > endTime) break; // Stop once the duration has expired.
        // Brightness reduced to 63, as tuned by the author.
        _strip.setPixelColor(0, _strip.ColorHSV(hue, 255, 63));
        _strip.show();
        delay(10);
    }
    _strip.clear();
    _strip.show();
}

void ESP32C3LEDManager::neoPixelOn() {
    _activateNeoPixelMode();
    _strip.setPixelColor(0, _strip.Color(255, 255, 255)); // White
    _strip.show();
}

void ESP32C3LEDManager::neoPixelOff() {
    _activateNeoPixelMode();
    _strip.clear();
    _strip.show();
}
