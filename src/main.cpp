#include <Arduino.h>
#include <RadioLib.h>
#include <U8g2lib.h>

const int VEXT_PIN = 36;
const int OLED_SDA = 17;
const int OLED_SCL = 18;
const int OLED_RST = 21;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, OLED_RST, OLED_SCL, OLED_SDA);

void setup() {
  Serial.begin(115200);
  pinMode(VEXT_PIN, OUTPUT);
  digitalWrite(VEXT_PIN, LOW);
  delay(100);

  display.begin();
  display.setFont(u8g2_font_ncenB14_tr);
  display.clearBuffer();
  display.drawStr(0, 32, "Hello uppsala!");
  display.sendBuffer();
}

void loop() {
  Serial.println("Redo");
  delay(1000);
}