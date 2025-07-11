// examples/BasicDemo/BasicDemo.ino

#include "ESP32C3LEDManager.h" // Die neu erstellte Klasse einbinden

// Eine Instanz der LED-Manager-Klasse erstellen.
// GPIO8 wird für beide LEDs verwendet, und es gibt 1 NeoPixel (die Onboard-RGB-LED).
ESP32C3LEDManager ledManager(8, 1); // sharedPin, numPixels

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32-C3 SuperMini Plus LED Demo (Klassen-Version) startet...");
  delay(1000); // Eine kurze Verzögerung, um der seriellen Verbindung Zeit zu geben.

  //  DEBUG
  if(!ledManager.isInitialized()) {
    Serial.println("⚠️ NeoPixel-Init fehlgeschlagen! - STOP");
    while (1);
  }
}

void loop() {
  Serial.println("\n--- NeoPixel Farbfading ---");
  ledManager.neoPixelColorFade(5000); // Fadet durch das Spektrum über 5 Sekunden.
  delay(1000);

  Serial.println("\n--- Blaue LED Ein-/Ausblenden (3 Mal) ---");
  for (int i = 0; i < 3; ++i) {
    ledManager.blueLEDFade(240, 1000); // Blendet in 1 Sekunde auf maximale sichere Helligkeit ein.
    delay(500);
    ledManager.blueLEDFade(0, 1000); // Blendet in 1 Sekunde aus.
    delay(500);
  }
  delay(1000);

  Serial.println("\n--- Blaue LED Rechteckwelle (2 Hz für 3 Sekunden) ---");
  ledManager.blueLEDSquareWave(2, 3000); // 2 Hz für 3 Sekunden.
  delay(1000);

  Serial.println("\n--- NeoPixel Rot ---");
  ledManager.neoPixelSetColor(255, 0, 0); // Setzt auf Rot.
  delay(1000);
  ledManager.neoPixelOff(); // Schaltet NeoPixel aus.
  delay(1000);

  Serial.println("\n--- Blaue LED Umschalten ---");
  ledManager.blueLEDToggle(); // Schaltet ein.
  delay(1000);
  ledManager.blueLEDToggle(); // Schaltet aus.
  delay(1000);

  Serial.println("\n--- Demo-Schleife abgeschlossen. Neustart... ---");
  delay(2000);
}
