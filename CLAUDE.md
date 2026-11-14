# CLAUDE.md — LoRa Offline Communication System

This file is the instruction file for Claude Code (and any Claude-based agent)
working in this repository. It mirrors `AGENTS.md` — the two files must be
kept in sync. **Read it before touching any file.** See the maintenance rule
at the bottom — it is mandatory.

## What this project is

An offline, peer-to-peer messaging system between **two Heltec WiFi LoRa 32
boards** (the devices we are using are LoRa 32 **V4** units; the PlatformIO env
is currently named/targeted for V3 — see Hardware below).

Behavior:

- Each device is **preloaded with a list of short messages** in the firmware.
- Each device has **two buttons**:
  - **PRG button (GPIO 0)** — short press steps to the next preloaded message
    (wraps around); **long press (≥ 1 s) transmits** the selected message
    over LoRa.
  - **Prev button (GPIO 4)** — steps to the previous message.
- When the **other device is in radio range**, it receives the packet and
  **displays the message on its OLED** (and logs it to Serial).
- No WiFi, internet, or broker involved — pure LoRa (SX1262) link. Both
  devices run the **same firmware**; either one can send or receive.

## Hardware

- Board: Heltec WiFi LoRa 32 (ESP32-S3 + SX1262 LoRa transceiver + 128x64
  SSD1306 OLED).
- `platformio.ini` currently defines env `heltec_wifi_lora_32_V3` with
  `board = heltec_wifi_lora_32_V3`. If the physical boards are V4 and the board
  id/pinout differ, update `board` in `platformio.ini` and the pins below.
- Typical Heltec LoRa 32 V3 pins (verify against your exact board revision
  before wiring; V4 may differ):
  - LoRa SX1262: SCK 9, MISO 11, MOSI 10, SS/CS 8, RST 12, DIO1/IRQ 14, BUSY 13
    (these are the `LORA_*` constants in `src/main.cpp`; the radio works with
    them on our V4 boards' V3-style pinout — verify if TX fails or range is
    poor, V4 also has an external PA that may need enable pins)
  - OLED (I2C): SDA 17, SCL 18, RST 21; Vext power GPIO 36 (active LOW)
  - Onboard PRG button: GPIO 0 (active LOW)
- Keep **all pin assignments in one config location** (a `pins.h` in `include/`
  or a clearly marked config block at the top of `src/main.cpp`) so they can be
  changed per board revision in one place. Current buttons: `PIN_BUTTON_NEXT` =
  GPIO 0 (onboard PRG, active LOW) and `PIN_BUTTON_PREV` = GPIO 4 (external
  button wired to GND, `INPUT_PULLUP`). **PRG short press = next message, PRG
  long press (≥ 1000 ms, `LONG_PRESS_MS`) = send the selected message.**

## Software stack and project layout

- PlatformIO + Arduino framework on ESP32.
- Dependencies (already in `platformio.ini` `lib_deps`):
  - `jgromes/RadioLib@^8.0.0` — SX1262 LoRa radio driver
  - `olikraus/U8g2@^2.36.18` — OLED graphics library
- Layout:
  - `platformio.ini` — env, board, framework, lib_deps
  - `README.md` — getting-started guide for humans: prerequisites (VS Code +
    PlatformIO IDE, or `pip install platformio` for the `pio` CLI), USB-serial
    driver, `pio run` / `pio run -t upload` / `pio device monitor`, wiring and
    usage of the two buttons (PRG short press = next, PRG long press = send,
    GPIO 4 = previous), radio settings, troubleshooting
  - `src/main.cpp` — firmware entry point. Current state: powers the OLED via
    Vext (GPIO 36 LOW), initializes U8g2 (SSD1306, HW I2C), and shows the
    selected entry from a preloaded table of 10 short messages (`MESSAGES[]`,
    e.g. "RUN", "HIDE", "BEHIND YOU") as an `n/10` index plus the message
    centered on screen. Two debounced buttons (200 ms, polled in `loop()`)
    cycle the selection with wrap-around: `PIN_BUTTON_NEXT` short press =
    next, `PIN_BUTTON_PREV` = previous. Long press on `PIN_BUTTON_NEXT` sends
    the selected message with `radio.transmit()` (RadioLib 8, `ConfigLoRa_t`
    + `radio.begin(config)`). A status line at the bottom of the OLED shows
    "Sending...", "Sent!", "Send error N" or "Radio error N" (init failure);
    the same is logged to Serial. Receive: the radio listens continuously
    (`radio.setIrqAction()` + `radio.startReceive()`); the DIO1 interrupt
    sets `packetReceived`, `loop()` reads the packet with `radio.readData()`,
    shows `<< MSG -45dBm` on the OLED and logs message, RSSI and SNR to
    Serial, then restarts receive. After each transmit the flag is cleared
    and receive is restarted. All pins and radio settings are constants at
    the top of the file.
  - `include/` — project headers (put shared config/pins here; currently only
    the PlatformIO `README`, no `pins.h` yet)
  - `lib/` — project-private libraries
  - `test/` — PlatformIO unit tests
  - `.vscode/` — VS Code / PlatformIO IDE settings
  - `.pio/` — build output and downloaded libraries. **Never edit, never commit.**

## Radio configuration (must match on BOTH devices)

- Frequency is region-dependent (e.g. 868.0 MHz in the EU, 915.0 MHz in the
  US, 433 MHz elsewhere) — set it once in the config block.
- Current settings (constants `LORA_*` at the top of `src/main.cpp`):
  868 MHz (EU), SF7, BW 125 kHz, CR 4/5, sync word
  `RADIOLIB_LORA_SYNC_WORD_PRIVATE` (0x12), 10 dBm TX power, preamble 8,
  TCXO 1.8 V (`radio.tcxoVoltage = Voltage1V8`). Both devices must use
  identical values or they will not hear each other.
- RadioLib 8 API note: `begin()` takes a `ConfigLoRa_t` struct with frequency
  and bandwidth in **Hz** (`RADIOLIB_UNIT_MEGA(868)`, `RADIOLIB_UNIT_KILO(125)`),
  not the old positional float arguments.
- Keep messages short — LoRa payloads at these settings should stay well under
  ~100 bytes.

## Firmware design (target behavior)

- **Message table**: a `const char* MESSAGES[]` array near the top of the code.
  Adding/removing a preloaded message = editing that array only.
  _Status: implemented — 10 short messages preloaded._
- **Buttons**: debounced (polled in `loop()`, 200 ms). PRG short press =
  next (registered on release), GPIO 4 = previous, both with wrap-around;
  PRG long press (≥ 1000 ms) calls `radio.transmit()` with the selected
  message (fires once per hold).
  _Status: cycling and send implemented._
- **Receive**: DIO1 interrupt (`setIrqAction`) sets a flag; `loop()` reads
  the payload as a string, shows it on the OLED with RSSI and prints it to
  Serial (with RSSI/SNR), then calls `startReceive()` again. CRC errors are
  shown as "RX CRC error" on the status line.
  _Status: implemented; not yet verified on hardware between board A and B._
- **OLED UI**: index `n/10` (top-left), selected message (large, centered,
  baseline y=34), last received message `<< MSG -45dBm` (y=52) and a status
  line (y=63: send result / RX error / radio error).
  _Status: implemented._

## Build / flash / monitor

- `pio run` — build
- `pio run -t upload` — flash (same firmware to both devices; with both
  boards connected use `--upload-port /dev/ttyUSB0` etc., see `pio device list`)
- `pio device monitor` — Serial monitor at 115200 baud
- In VS Code use the PlatformIO IDE extension (see `.vscode/extensions.json`).
- See `README.md` for the full getting-started guide (environment setup, USB
  flashing, button usage, troubleshooting).

## Git hygiene

- `.gitignore` contains `.pio/`, and the artifacts under `.pio/` have been
  removed from the git index (files kept on disk) — they are no longer tracked.
  Never add or modify anything under `.pio/`.
- Commit only source, config, and docs.

## Documentation maintenance rule (mandatory)

Any agent working on the files in this repository **must review and update
both `AGENTS.md` and `CLAUDE.md`** so they stay accurate and up to date as the
project evolves:

1. Before making changes, read `AGENTS.md` and `CLAUDE.md`.
2. After creating, modifying, or deleting any project file (code, config,
   pins, commands, dependencies, behavior), update **both** files to reflect
   the current state — project layout, hardware/pins, radio settings, commands,
   and behavior.
3. The two files must be kept **in sync with each other**: any change made to
   one must be mirrored in the other (except for the tool-specific preamble at
   the top of each file).
