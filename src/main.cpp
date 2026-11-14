#include <Arduino.h>
#include <RadioLib.h>
#include <U8g2lib.h>

// ---- Pin configuration (Heltec WiFi LoRa 32 V3; verify for V4) ----
const int VEXT_PIN = 36;  // OLED power, active LOW
const int OLED_SDA = 17;
const int OLED_SCL = 18;
const int OLED_RST = 21;

const int PIN_BUTTON_NEXT = 0;  // onboard PRG button, active LOW
const int PIN_BUTTON_PREV = 4;  // external button wired to GND, active LOW
// NOTE: LoRa sending is not implemented yet. Later, one of these buttons
// (or a long-press) will transmit the selected message — see AGENTS.md.

// ---- Preloaded short messages ----
const char* MESSAGES[] = {
  "RUN",
  "HIDE",
  "BEHIND YOU",
  "HELP",
  "STOP",
  "WAIT",
  "GO",
  "COME",
  "OK",
  "LOOK",
};
const int MESSAGE_COUNT = sizeof(MESSAGES) / sizeof(MESSAGES[0]);

int currentMessage = 0;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, OLED_RST, OLED_SCL, OLED_SDA);

// ---- Button debounce state ----
const unsigned long DEBOUNCE_MS = 200;
bool rawNext = HIGH, stableNext = HIGH;
unsigned long lastChangeNext = 0;
bool rawPrev = HIGH, stablePrev = HIGH;
unsigned long lastChangePrev = 0;

// Returns true once per press (buttons are active LOW) once the signal has
// been stable for DEBOUNCE_MS.
bool buttonPressed(int pin, bool &raw, bool &stable, unsigned long &lastChange) {
  bool reading = digitalRead(pin);
  unsigned long now = millis();
  if (reading != raw) {
    raw = reading;
    lastChange = now;
  }
  if (reading == stable || now - lastChange < DEBOUNCE_MS) return false;
  stable = reading;
  return reading == LOW;
}

void drawScreen() {
  display.clearBuffer();
  // index indicator, small font, top-left
  display.setFont(u8g2_font_6x10_tr);
  char indexStr[8];
  snprintf(indexStr, sizeof(indexStr), "%d/%d", currentMessage + 1, MESSAGE_COUNT);
  display.drawStr(0, 10, indexStr);
  // message, big font, horizontally centered
  display.setFont(u8g2_font_ncenB14_tr);
  const char* msg = MESSAGES[currentMessage];
  int w = (int)display.getStrWidth(msg);
  int x = (128 - w) / 2;
  if (x < 0) x = 0;
  display.drawStr(x, 44, msg);
  display.sendBuffer();
}

void setup() {
  Serial.begin(115200);
  pinMode(VEXT_PIN, OUTPUT);
  digitalWrite(VEXT_PIN, LOW);
  delay(100);

  pinMode(PIN_BUTTON_NEXT, INPUT_PULLUP);
  pinMode(PIN_BUTTON_PREV, INPUT_PULLUP);

  display.begin();
  drawScreen();
}

void loop() {
  if (buttonPressed(PIN_BUTTON_NEXT, rawNext, stableNext, lastChangeNext)) {
    currentMessage = (currentMessage + 1) % MESSAGE_COUNT;
    Serial.printf("Selected %d/%d: %s\n", currentMessage + 1, MESSAGE_COUNT, MESSAGES[currentMessage]);
    drawScreen();
  }
  if (buttonPressed(PIN_BUTTON_PREV, rawPrev, stablePrev, lastChangePrev)) {
    currentMessage = (currentMessage - 1 + MESSAGE_COUNT) % MESSAGE_COUNT;
    Serial.printf("Selected %d/%d: %s\n", currentMessage + 1, MESSAGE_COUNT, MESSAGES[currentMessage]);
    drawScreen();
  }
}
