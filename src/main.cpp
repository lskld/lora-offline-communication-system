#include <Arduino.h>
#include <RadioLib.h>
#include <U8g2lib.h>

void setup() {
  Serial.begin(115200);
}

void loop() {
  Serial.println("Redo");
  delay(1000);
}