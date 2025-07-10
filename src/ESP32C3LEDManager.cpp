#include "ESP32C3LEDManager.h"

// Konstruktor
ESP32C3LEDManager::ESP32C3LEDManager(int sharedPin, int numPixels)
    : _sharedPin(sharedPin), _numPixels(numPixels),
      _strip(numPixels, sharedPin, NEO_GRB + NEO_KHZ800) {
    // Initialer Setup im Konstruktor
    // Sicherstellen, dass der gemeinsame Pin initial als digitaler Ausgang konfiguriert und AUS ist.
    // Die blaue LED ist NICHT invertiert: LOW = AUS, HIGH = AN.
    pinMode(_sharedPin, OUTPUT);
    digitalWrite(_sharedPin, LOW);

    // NeoPixel initialisieren (konfiguriert RMT für den gemeinsamen Pin).
    _strip.begin();
    _strip.show(); // Sicherstellen, dass NeoPixel initial AUS ist.
}

// Interne Helferfunktion: Bereitet den Pin für NeoPixel-Operationen vor
void ESP32C3LEDManager::_activateNeoPixelMode() {
    // LEDC-Peripherie vom Pin trennen, falls aktiv.
    ledcDetach(_sharedPin);
    // Sicherstellen, dass der Pin als digitaler Ausgang konfiguriert und auf LOW ist.
    // Dies ist entscheidend, um zu verhindern, dass die NeoPixel-LED zu Weiß glitcht.
    pinMode(_sharedPin, OUTPUT);
    digitalWrite(_sharedPin, LOW);
    delay(50); // Kurze Verzögerung, damit sich der Pin-Zustand stabilisiert.

    // NeoPixel-Bibliothek erneut initialisieren (bindet RMT wieder an den Pin).
    _strip.begin();
}

// Interne Helferfunktion: Bereitet den Pin für blaue LED (LEDC)-Operationen vor
void ESP32C3LEDManager::_activateBlueLEDMode() {
    // NeoPixel ausschalten und RMT-Ressourcen freigeben (soweit möglich).
    _strip.clear();
    _strip.show();
    // Sicherstellen, dass der Pin digital auf LOW ist, bevor LEDC angehängt wird, um Glitches zu vermeiden.
    pinMode(_sharedPin, OUTPUT);
    digitalWrite(_sharedPin, LOW);
    delay(50); // Kurze Verzögerung, damit sich der Pin-Zustand stabilisiert.

    // LEDC-Peripherie an den Pin anhängen (konfiguriert den Pin für PWM).
    ledcAttach(_sharedPin, _blueLEDFreq, _blueLEDResolution);
}

// Interne Helferfunktion: Räumt nach blauen LED (LEDC)-Operationen auf und bereitet für NeoPixel vor
void ESP32C3LEDManager::_deactivateBlueLEDMode() {
    // Sicherstellen, dass die blaue LED über LEDC ausgeschaltet ist.
    ledcWrite(_sharedPin, 0); // 0 = AUS für nicht-invertierte blaue LED.
    // LEDC-Peripherie vom Pin trennen.
    ledcDetach(_sharedPin);

    // Den Pin sofort wieder für NeoPixel-Operationen vorbereiten und NeoPixel ausschalten.
    _activateNeoPixelMode();
    _strip.clear();
    _strip.show();
}

// --- Implementierung der Funktionen zur Steuerung der blauen LED ---
void ESP32C3LEDManager::blueLEDOn() {
    _activateBlueLEDMode();
    ledcWrite(_sharedPin, _blueLEDMaxBrightness); // Einschalten auf maximale sichere Helligkeit.
}

void ESP32C3LEDManager::blueLEDOff() {
    _activateBlueLEDMode(); // LEDC anhängen, um über PWM auszuschalten.
    ledcWrite(_sharedPin, 0); // Ausschalten.
    _deactivateBlueLEDMode(); // LEDC trennen und für NeoPixel vorbereiten.
}

void ESP32C3LEDManager::blueLEDToggle() {
    _activateBlueLEDMode();
    // Aktuellen Duty Cycle lesen. Wenn 0 (aus), einschalten. Sonst ausschalten.
    if (ledcRead(_sharedPin) == 0) {
        ledcWrite(_sharedPin, _blueLEDMaxBrightness);
    } else {
        ledcWrite(_sharedPin, 0);
        _deactivateBlueLEDMode(); // Wenn ausgeschaltet wird, aufräumen.
    }
}

void ESP32C3LEDManager::blueLEDFade(int targetBrightness, int durationMs) {
    _activateBlueLEDMode();
    int currentBrightness = ledcRead(_sharedPin); // Aktuelle Helligkeit abrufen.
    // Sicherstellen, dass die Zielhelligkeit innerhalb der sicheren Grenzen liegt.
    if (targetBrightness > _blueLEDMaxBrightness) targetBrightness = _blueLEDMaxBrightness;
    if (targetBrightness < 0) targetBrightness = 0;

    int steps = abs(targetBrightness - currentBrightness);
    if (steps == 0) {
        if (targetBrightness == 0) _deactivateBlueLEDMode(); // Wenn bereits aus, aufräumen.
        return;
    }
    int delayPerStep = durationMs / steps;
    if (delayPerStep == 0) delayPerStep = 1; // Mindestverzögerung.

    if (targetBrightness > currentBrightness) {
        for (int brightness = currentBrightness; brightness <= targetBrightness; brightness++) {
            ledcWrite(_sharedPin, brightness);
            delay(delayPerStep);
        }
    } else {
        for (int brightness = currentBrightness; brightness >= targetBrightness; brightness--) {
            ledcWrite(_sharedPin, brightness);
            delay(delayPerStep);
        }
    }
    // Wenn auf AUS gefadet wird, aufräumen. Sonst LEDC angehängt lassen.
    if (targetBrightness == 0) {
        _deactivateBlueLEDMode();
    }
}

void ESP32C3LEDManager::blueLEDSquareWave(int frequencyHz, int durationMs) {
    _activateBlueLEDMode();
    long startTime = millis();
    long endTime = startTime + durationMs;
    unsigned long halfPeriodMs = 1000 / (2 * frequencyHz); // Halbe Periode für AN/AUS.

    while (millis() < endTime) {
        ledcWrite(_sharedPin, _blueLEDMaxBrightness); // AN
        delay(halfPeriodMs);
        if (millis() >= endTime) break; // Prüfen, ob Dauer während AN-Zustand abgelaufen ist.
        ledcWrite(_sharedPin, 0); // AUS
        delay(halfPeriodMs);
    }
    _deactivateBlueLEDMode(); // Sicherstellen, dass LED aus ist und aufräumen.
}

// --- Implementierung der Funktionen zur Steuerung der NeoPixel-LED ---
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
    // Schrittweite für den Farbton anpassen, um das gesamte Spektrum abzudecken.
    int hueStep = 65536 / (durationMs / 10);
    if (hueStep == 0) hueStep = 1; // Mindestens 1 Schritt sicherstellen.

    for (int hue = 0; hue < 65536; hue += hueStep) {
        if (millis() > endTime) break; // Stoppen, wenn Dauer abgelaufen ist.
        // Helligkeit auf 63 reduziert, wie vom Benutzer optimiert.
        _strip.setPixelColor(0, _strip.ColorHSV(hue, 255, 63));
        _strip.show();
        delay(10);
    }
    _strip.clear();
    _strip.show();
}

void ESP32C3LEDManager::neoPixelOn() {
    _activateNeoPixelMode();
    _strip.setPixelColor(0, _strip.Color(255, 255, 255)); // Weiß
    _strip.show();
}

void ESP32C3LEDManager::neoPixelOff() {
    _activateNeoPixelMode();
    _strip.clear();
    _strip.show();
}