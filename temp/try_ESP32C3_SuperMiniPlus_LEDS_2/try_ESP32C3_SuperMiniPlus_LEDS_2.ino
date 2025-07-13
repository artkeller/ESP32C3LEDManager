#include <Adafruit_NeoPixel.h>  // Bibliothek für NeoPixel-LEDs

// Definitionen für die LEDs
#define NEOPIXEL_PIN 8  // GPIO-Pin für die NeoPixel-LED (und die blaue LED)
#define BLUE_LED_PIN 8  // GPIO-Pin für die blaue LED (und die NeoPixel-LED)
#define NUM_PIXELS 1    // Es ist eine einzelne Onboard-RGB-LED

// Erstellen eines NeoPixel-Objekts
// Parameter 1 = Anzahl der Pixel im Streifen
// Parameter 2 = Arduino-Pin-Nummer (die meisten ESP32-Pins funktionieren)
// Parameter 3 = Pixel-Typ-Flags (NEO_GRB ist am häufigsten für WS2812B)
Adafruit_NeoPixel strip(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Definitionen für das Fading der blauen LED (PWM)
const int blueLEDFreq = 1000;     // 1 kHz Frequenz (vom Benutzer optimiert)
const int blueLEDResolution = 8;  // 8-Bit-Auflösung (Werte von 0 bis 255)
// Maximale Helligkeit für die blaue LED. Begrenzt auf 240, um NeoPixel-Trigger zu vermeiden.
const int blueLEDMaxBrightness = 240; // max. Tastrate ca. 94% (vom Benutzer optimiert)


void setup() {
  // Blauen LED-Pin als einfachen digitalen Ausgang konfigurieren
  // und sicherstellen, dass die blaue LED initial aus ist (LOW = AUS, da nicht invertiert)
  pinMode(BLUE_LED_PIN, OUTPUT);
  digitalWrite(BLUE_LED_PIN, LOW);

  Serial.begin(115200);  // Serielle Kommunikation starten
  Serial.println("ESP32-C3 SuperMini Plus LED Demo startet...");
  delay(2000); 

  // NeoPixel initialisieren. Dies konfiguriert GPIO8 für die RMT-Peripherie.
  strip.begin();
  strip.show();  // Alle Pixel auf 'aus' setzen
  // Eine kurze Pause, um sicherzustellen, dass sich alles gesetzt hat.
  delay(100);
}

void loop() {
  Serial.println("Starte NeoPixel-Regenbogen-Fade...");

  // Sicherstellen, dass die blaue LED aus ist, bevor NeoPixel startet.
  // Da NeoPixel den Pin über RMT übernimmt, sollte dies ausreichen.
  // Die blaue LED ist nicht invertiert, LOW = AUS.
  digitalWrite(BLUE_LED_PIN, LOW);

  // NeoPixel durch das Farbspektrum faden
  for (int hue = 0; hue < 65536; hue += 128) {  // Hue-Werte von 0 bis 65535, Schrittweite für sanften Übergang
    // Setze die Farbe des ersten Pixels (Index 0)
    // ColorHSV(hue, Sättigung, Helligkeit)
    // Helligkeit auf 63 reduziert (vom Benutzer optimiert)
    strip.setPixelColor(0, strip.ColorHSV(hue, 255, 63));  // Volle Sättigung, reduzierte Helligkeit
    strip.show();                                           // Aktualisiere die LED
    delay(10);                                              // Kleine Verzögerung für sanfte Animation
  }
  delay(2000);  // Kurze Pause

  Serial.println("NeoPixel-Fade abgeschlossen. Schalte NeoPixel aus.");
  strip.clear();  // Alle NeoPixel ausschalten
  strip.show();
  delay(2999);  // Kurze Pause

  Serial.println("Starte Fading der blauen LED (3 Mal)...");
  // Jetzt binden wir die LEDC-Peripherie an den Pin, um die blaue LED zu faden.
  // Dies konfiguriert den Pin für PWM und übernimmt die Kontrolle von RMT.
  ledcAttach(BLUE_LED_PIN, blueLEDFreq, blueLEDResolution);
  // Sicherstellen, dass die blaue LED initial aus ist (0 ist aus bei nicht-invertiert)
  ledcWrite(BLUE_LED_PIN, 0);

  // Blaue LED dreimal faden
  for (int i = 0; i < 3; i++) {
    // Fade-in (von aus zu hellst)
    // 0 (aus) bis blueLEDMaxBrightness (hell)
    for (int brightness = 0; brightness <= blueLEDMaxBrightness; brightness++) {
      ledcWrite(BLUE_LED_PIN, brightness);
      delay(5);
    }
    // Fade-out (von hellst zu aus)
    // blueLEDMaxBrightness (hell) bis 0 (aus)
    for (int brightness = blueLEDMaxBrightness; brightness >= 0; brightness--) {
      ledcWrite(BLUE_LED_PIN, brightness);
      delay(5);
    }
    delay(200);  // Pause zwischen den Fades
  }

  Serial.println("Fading der blauen LED abgeschlossen. Sketch beendet.");
  // Sicherstellen, dass die blaue LED am Ende aus ist
  ledcWrite(BLUE_LED_PIN, 0);  // 0 ist aus bei nicht-invertiert

  // WICHTIG: LEDC-Peripherie vom Pin trennen.
  ledcDetach(BLUE_LED_PIN);

  // ZUSÄTZLICH WICHTIG: Den Pin explizit auf LOW setzen, um die NeoPixel-LED zu "beruhigen".
  // Dies ist entscheidend, um den weißen Blitz zu verhindern, da der Pin nach ledcDetach
  // kurzzeitig undefiniert sein könnte.
  pinMode(NEOPIXEL_PIN, OUTPUT); // Sicherstellen, dass es ein digitaler Ausgang ist
  digitalWrite(NEOPIXEL_PIN, LOW); // Pin auf LOW setzen
  delay(50); // Eine kurze Pause, damit sich der Pin-Zustand stabilisiert

  // WICHTIG: NeoPixel-Bibliothek erneut initialisieren und ausschalten,
  // um sicherzustellen, dass der Pin wieder von RMT kontrolliert wird
  // und die NeoPixel-LED definitiv aus ist.
  strip.begin();  // Re-initialisiert den Pin für RMT
  strip.clear();  // Löscht alle Pixeldaten
  strip.show();   // Sendet das "alles aus"-Signal an die NeoPixel-LED

  // Endlosschleife, um den Sketch zu beenden
  //while (true) {
  //  delay(1000);
 // }

 delay(2000);
}