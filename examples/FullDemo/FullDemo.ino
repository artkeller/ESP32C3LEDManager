// examples/FullDemo/FullDemo.ino

#include "ESP32C3LEDManager.h" 

// Eine Instanz der LED-Manager-Klasse erstellen.
// GPIO8 wird für beide LEDs verwendet, und es gibt 1 NeoPixel (die Onboard-RGB-LED).
ESP32C3LEDManager ledManager(8, 1); // sharedPin, numPixels

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32-C3 SuperMini Plus LED Full Demo (Klassen-Version) startet...");
  delay(1000); // Eine kurze Verzögerung, um der seriellen Verbindung Zeit zu geben.

  //  DEBUG
  if(!ledManager.isInitialized()) {
    Serial.println("⚠️ NeoPixel-Init fehlgeschlagen! - STOP");
    while (1);
  }
}

void loop() {
  Serial.println("\n--- Phase 1: NeoPixel Farbfading ---");
  Serial.println("NeoPixel fadet durch das Spektrum, blaue LED bleibt aus.");
  ledManager.neoPixelColorFade(5000); // Fadet durch das Spektrum über 5 Sekunden.
  delay(1000);

  Serial.println("\n--- Phase 2: Blaue LED Einblenden, NeoPixel Rot ---");
  Serial.println("Blaue LED blendet ein, NeoPixel wird Rot.");
  ledManager.neoPixelSetColor(255, 0, 0); // NeoPixel auf Rot setzen
  ledManager.blueLEDFade(240, 1000); // Blaue LED in 1 Sekunde auf maximale sichere Helligkeit einblenden.
  delay(1000);

  Serial.println("\n--- Phase 3: Blaue LED Ausblenden, NeoPixel Grün ---");
  Serial.println("Blaue LED blendet aus, NeoPixel wird Grün.");
  ledManager.neoPixelSetColor(0, 255, 0); // NeoPixel auf Grün setzen
  ledManager.blueLEDFade(0, 1000); // Blaue LED in 1 Sekunde ausblenden.
  delay(1000);

  Serial.println("\n--- Phase 4: Blaue LED AN, NeoPixel Blau ---");
  Serial.println("Blaue LED geht AN, NeoPixel wird Blau.");
  ledManager.neoPixelSetColor(0, 0, 255); // NeoPixel auf Blau setzen
  ledManager.blueLEDOn(); // Blaue LED direkt einschalten.
  delay(1000);

  Serial.println("\n--- Phase 5: Blaue LED AUS, NeoPixel AN (Weiß) ---");
  Serial.println("Blaue LED geht AUS, NeoPixel wird Weiß.");
  ledManager.neoPixelOn(); // NeoPixel auf Weiß setzen.
  ledManager.blueLEDOff(); // Blaue LED direkt ausschalten.
  delay(1000);

  Serial.println("\n--- Phase 6: Blaue LED Umschalten, NeoPixel AUS ---");
  Serial.println("Blaue LED wird umgeschaltet, NeoPixel geht AUS.");
  ledManager.neoPixelOff(); // NeoPixel ausschalten.
  ledManager.blueLEDToggle(); // Blaue LED umschalten (sollte AN gehen).
  delay(1000);
  ledManager.blueLEDToggle(); // Blaue LED umschalten (sollte AUS gehen).
  delay(1000);

  Serial.println("\n--- Phase 7: Blaue LED Rechteckwelle ---");
  Serial.println("Blaue LED blinkt in Rechteckwelle, NeoPixel bleibt AUS.");
  ledManager.blueLEDSquareWave(3, 3000); // 3 Hz für 3 Sekunden.
  delay(1000);

  Serial.println("\n--- Phase 8: NeoPixel HSV-Farbe (Gelb) ---");
  Serial.println("NeoPixel wird Gelb (HSV), blaue LED bleibt AUS.");
  ledManager.neoPixelSetColorHSV(10000, 255, 63); // Gelb (H=10000), volle Sättigung, reduzierte Helligkeit.
  delay(1500);
  ledManager.neoPixelOff(); // NeoPixel ausschalten.
  delay(1000);

  Serial.println("\n--- Demo-Schleife abgeschlossen. Neustart... ---");
  delay(2000);
}
