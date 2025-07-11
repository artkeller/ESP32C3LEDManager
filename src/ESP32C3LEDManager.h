#ifndef ESP32C3LEDMANAGER_H
#define ESP32C3LEDMANAGER_H

#include <Adafruit_NeoPixel.h>
#include <Arduino.h> // Für uint8_t, uint16_t, etc.

class ESP32C3LEDManager {
public:
    // Konstruktor: Initialisiert den Manager für den gemeinsamen GPIO-Pin und die Anzahl der NeoPixel.
    // sharedPin: Der GPIO-Pin, der von beiden LEDs genutzt wird (hier GPIO8).
    // numPixels: Die Anzahl der NeoPixel-LEDs (hier 1 für die Onboard-RGB-LED).
    ESP32C3LEDManager(int sharedPin, int numPixels = 1);

    // --- Funktionen zur Steuerung der blauen LED ---
    // Schaltet die blaue LED ein (auf die maximale sichere Helligkeit).
    void blueLEDOn();
    // Schaltet die blaue LED aus.
    void blueLEDOff();
    // Schaltet die blaue LED um (zwischen AUS und maximaler sicherer Helligkeit).
    void blueLEDToggle();
    // Fadet die blaue LED zu einer Zielhelligkeit über eine bestimmte Dauer.
    // targetBrightness: Zielhelligkeit (0-240, um NeoPixel-Trigger zu vermeiden).
    // durationMs: Dauer des Fading in Millisekunden.
    void blueLEDFade(int targetBrightness, int durationMs);
    // Lässt die blaue LED in einer Rechteckwelle (blinkend) für eine bestimmte Dauer leuchten.
    // frequencyHz: Frequenz des Blinkens in Hertz.
    // durationMs: Gesamtdauer des Blinkens in Millisekunden.
    void blueLEDSquareWave(int frequencyHz, int durationMs);

    // --- Funktionen zur Steuerung der NeoPixel-LED ---
    // Setzt die Farbe der NeoPixel-LED mit RGB-Werten.
    // r, g, b: Rot-, Grün-, Blau-Werte (0-255).
    void neoPixelSetColor(uint8_t r, uint8_t g, uint8_t b);
    // Setzt die Farbe der NeoPixel-LED mit HSV-Werten.
    // hue: Farbton (0-65535).
    // sat: Sättigung (0-255).
    // val: Helligkeit (0-255).
    void neoPixelSetColorHSV(uint16_t hue, uint8_t sat, uint8_t val);
    // Fadet die NeoPixel-LED durch das Farbspektrum über eine bestimmte Dauer.
    // durationMs: Dauer des Fading in Millisekunden.
    void neoPixelColorFade(int durationMs);
    // Schaltet die NeoPixel-LED auf volle weiße Helligkeit.
    void neoPixelOn();
    // Schaltet die NeoPixel-LED aus.
    void neoPixelOff();
    // DEBUG INFO
    bool neoPixelIsInitialized() const;

private:
    int _sharedPin;
    int _numPixels;
    Adafruit_NeoPixel _strip; // Das Adafruit NeoPixel Objekt als Member
    bool _stripInitialized = false; // DEBUG INFO

    // LEDC-Parameter für die blaue LED (vom Benutzer optimiert)
    const int _blueLEDFreq = 1000;     // 1 kHz Frequenz
    const int _blueLEDResolution = 8;  // 8-Bit-Auflösung (Werte von 0 bis 255)
    // Maximale Helligkeit für die blaue LED, um NeoPixel-Trigger zu vermeiden
    const int _blueLEDMaxBrightness = 240;

    // Interne Helferfunktionen zur Verwaltung des gemeinsamen Pins
    // Bereitet den Pin für NeoPixel-Operationen vor (aktiviert RMT, deaktiviert LEDC).
    void _activateNeoPixelMode();
    // Bereitet den Pin für blaue LED (LEDC/PWM)-Operationen vor (aktiviert LEDC, deaktiviert RMT).
    void _activateBlueLEDMode();
    // Räumt nach blauen LED (LEDC)-Operationen auf und bereitet den Pin wieder für NeoPixel vor.
    void _deactivateBlueLEDMode();
};

#endif // ESP32C3LEDMANAGER_H
