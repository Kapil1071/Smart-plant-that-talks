#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

#define SOIL_PIN  A0
#define MP3_RX    10
#define MP3_TX    11

SoftwareSerial mp3Serial(MP3_RX, MP3_TX);
DFRobotDFPlayerMini mp3;

unsigned long lastSend = 0, lastAlert = 0;

void setup() {
  Serial.begin(9600);
  mp3Serial.begin(9600);
  if (mp3.begin(mp3Serial)) mp3.volume(25);
  Serial.println("{\"status\":\"ready\"}");
}

void loop() {
  int soil = constrain(map(analogRead(SOIL_PIN), 1023, 300, 0, 100), 0, 100);

  if (millis() - lastSend > 5000) {
    Serial.print("{\"soil\":"); Serial.print(soil);
    Serial.println("}");
    lastSend = millis();
  }

  if (millis() - lastAlert > 60000) {
    if (soil < 20) { mp3.play(1); lastAlert = millis(); }
  }
  delay(1000);
}
