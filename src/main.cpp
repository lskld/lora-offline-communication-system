#include <Arduino.h>
#include <RadioLib.h>
#include <U8g2lib.h>

// ---- Pin configuration (Heltec WiFi LoRa 32 V3; verify for V4) ----
const int VEXT_PIN = 36;  // OLED power, active LOW
const int OLED_SDA = 17;
const int OLED_SCL = 18;
const int OLED_RST = 21;

const int PIN_BUTTON_NEXT = 0;  // onboard PRG button, active LOW; long-press = send
const int PIN_BUTTON_PREV = 4;  // external button wired to GND, active LOW

// SX1262 LoRa radio (SPI)
const int LORA_SCK = 9;
const int LORA_MISO = 11;
const int LORA_MOSI = 10;
const int LORA_CS = 8;
const int LORA_RST = 12;
const int LORA_DIO1 = 14;
const int LORA_BUSY = 13;

// ---- Radio configuration (must be identical on BOTH devices) ----
const uint32_t LORA_FREQ_HZ = RADIOLIB_UNIT_MEGA(868);  // EU
const uint32_t LORA_BW_HZ = RADIOLIB_UNIT_KILO(125);
const uint8_t LORA_SF = 7;
const uint8_t LORA_CR = 5;  // 4/5
const uint8_t LORA_SYNC_WORD = RADIOLIB_LORA_SYNC_WORD_PRIVATE;
const int8_t LORA_TX_POWER_DBM = 10;
const uint16_t LORA_PREAMBLE_LEN = 8;
const RadioLibTCXOVoltage_t LORA_TCXO_VOLTAGE = Voltage1V8;

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

SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY);
bool radioReady = false;

// Set from the DIO1 interrupt when the radio has received a packet
volatile bool packetReceived = false;

// Short status line shown at the bottom of the OLED (e.g. "Sent!")
char statusText[24] = "";
// Last received message line, e.g. "<< HELP -45dBm"
char receivedText[24] = "";

// ---- Button debounce state ----
const unsigned long DEBOUNCE_MS = 200;
const unsigned long LONG_PRESS_MS = 1000;
bool rawNext = HIGH, stableNext = HIGH;
unsigned long lastChangeNext = 0;
bool longPressFired = false;
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
  display.drawStr(x, 34, msg);
  // last received message and status line, small font, bottom
  display.setFont(u8g2_font_6x10_tr);
  display.drawStr(0, 52, receivedText);
  display.drawStr(0, 63, statusText);
  display.sendBuffer();
}

void selectMessage(int index) {
  currentMessage = (index + MESSAGE_COUNT) % MESSAGE_COUNT;
  statusText[0] = '\0';
  Serial.printf("Selected %d/%d: %s\n", currentMessage + 1, MESSAGE_COUNT, MESSAGES[currentMessage]);
  drawScreen();
}

void sendCurrentMessage() {
  const char* msg = MESSAGES[currentMessage];
  if (!radioReady) {
    Serial.println("Send skipped: radio not initialized");
    snprintf(statusText, sizeof(statusText), "Radio not ready");
    drawScreen();
    return;
  }
  snprintf(statusText, sizeof(statusText), "Sending...");
  drawScreen();
  int state = radio.transmit(msg);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.printf("Sent: %s\n", msg);
    snprintf(statusText, sizeof(statusText), "Sent!");
  } else {
    Serial.printf("Send failed, code %d\n", state);
    snprintf(statusText, sizeof(statusText), "Send error %d", state);
  }
  // transmit() also fires DIO1 (TX done); ignore that and go back to listening
  packetReceived = false;
  radio.startReceive();
  drawScreen();
}

void IRAM_ATTR onPacketReceived() {
  packetReceived = true;
}

void handleReceivedPacket() {
  packetReceived = false;
  String msg;
  int state = radio.readData(msg);
  if (state == RADIOLIB_ERR_NONE) {
    float rssi = radio.getRSSI();
    float snr = radio.getSNR();
    Serial.printf("Received: %s (RSSI %.0f dBm, SNR %.1f dB)\n", msg.c_str(), rssi, snr);
    snprintf(receivedText, sizeof(receivedText), "<< %s %.0fdBm", msg.c_str(), rssi);
  } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
    Serial.println("Receive failed: CRC mismatch");
    snprintf(statusText, sizeof(statusText), "RX CRC error");
  } else {
    Serial.printf("Receive failed, code %d\n", state);
    snprintf(statusText, sizeof(statusText), "RX error %d", state);
  }
  radio.startReceive();
  drawScreen();
}

// PRG button: short press (released before LONG_PRESS_MS) = next message,
// long press = send the selected message (fires once while held).
void handleNextButton() {
  bool reading = digitalRead(PIN_BUTTON_NEXT);
  unsigned long now = millis();
  if (reading != rawNext) {
    rawNext = reading;
    lastChangeNext = now;
  }
  if (reading != stableNext && now - lastChangeNext >= DEBOUNCE_MS) {
    stableNext = reading;
    if (stableNext == LOW) {
      longPressFired = false;
    } else if (!longPressFired) {
      selectMessage(currentMessage + 1);
    }
  }
  if (stableNext == LOW && !longPressFired && now - lastChangeNext >= LONG_PRESS_MS) {
    longPressFired = true;
    sendCurrentMessage();
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(VEXT_PIN, OUTPUT);
  digitalWrite(VEXT_PIN, LOW);
  delay(100);

  pinMode(PIN_BUTTON_NEXT, INPUT_PULLUP);
  pinMode(PIN_BUTTON_PREV, INPUT_PULLUP);

  display.begin();

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  ConfigLoRa_t config;
  config.frequency = LORA_FREQ_HZ;
  config.bandwidth = LORA_BW_HZ;
  config.spreadingFactor = LORA_SF;
  config.codingRate = LORA_CR;
  config.syncWord = LORA_SYNC_WORD;
  config.power = LORA_TX_POWER_DBM;
  config.preambleLength = LORA_PREAMBLE_LEN;
  radio.tcxoVoltage = LORA_TCXO_VOLTAGE;
  int state = radio.begin(config);
  if (state == RADIOLIB_ERR_NONE) {
    radio.setIrqAction(onPacketReceived);
    state = radio.startReceive();
  }
  if (state == RADIOLIB_ERR_NONE) {
    radioReady = true;
    Serial.println("Radio ready, listening");
  } else {
    Serial.printf("Radio init failed, code %d\n", state);
    snprintf(statusText, sizeof(statusText), "Radio error %d", state);
  }

  drawScreen();
}

void loop() {
  if (packetReceived) {
    handleReceivedPacket();
  }
  handleNextButton();
  if (buttonPressed(PIN_BUTTON_PREV, rawPrev, stablePrev, lastChangePrev)) {
    selectMessage(currentMessage - 1);
  }
}
