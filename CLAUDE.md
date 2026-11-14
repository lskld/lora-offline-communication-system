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
  - **Cycle button** — steps through the preloaded messages (wraps around) and
    shows the currently selected one on the OLED.
  - **Send button** — transmits the currently selected message over LoRa.
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
  - LoRa SX1262: SCK 9, MISO 11, MOSI 10, SS/CS 8, RST 12, DIO0/IRQ 14, BUSY 13
  - OLED (I2C): SDA 17, SCL 18, RST 21; Vext power GPIO 36 (active LOW)
  - Onboard PRG button: GPIO 0 (active LOW)
- Keep **all pin assignments in one config location** (a `pins.h` in `include/`
  or a clearly marked config block at the top of `src/main.cpp`) so they can be
  changed per board revision in one place. Define `PIN_BUTTON_CYCLE` and
  `PIN_BUTTON_SEND` there (e.g. PRG button GPIO 0 for one, a free GPIO with
  `INPUT_PULLUP` for the other).

## Software stack and project layout

- PlatformIO + Arduino framework on ESP32.
- Dependencies (already in `platformio.ini` `lib_deps`):
  - `jgromes/RadioLib@^8.0.0` — SX1262 LoRa radio driver
  - `olikraus/U8g2@^2.36.18` — OLED graphics library
- Layout:
  - `platformio.ini` — env, board, framework, lib_deps
  - `src/main.cpp` — firmware entry point (currently a stub printing "Redo";
    the real firmware is not implemented yet)
  - `include/` — project headers (put shared config/pins here)
  - `lib/` — project-private libraries
  - `test/` — PlatformIO unit tests
  - `.vscode/` — VS Code / PlatformIO IDE settings
  - `.pio/` — build output and downloaded libraries. **Never edit, never commit.**

## Radio configuration (must match on BOTH devices)

- Frequency is region-dependent (e.g. 868.0 MHz in the EU, 915.0 MHz in the
  US, 433 MHz elsewhere) — set it once in the config block.
- Modem settings (suggested starting point): SF7, BW 125 kHz, CR 4/5, default
  sync word, modest TX power. Define all of them as constants in the same
  config location; both devices must use identical values or they will not
  hear each other.
- Keep messages short — LoRa payloads at these settings should stay well under
  ~100 bytes.

## Firmware design (target behavior)

- **Message table**: a `const char* MESSAGES[]` array near the top of the code.
  Adding/removing a preloaded message = editing that array only.
- **Buttons**: debounced (poll in `loop()` or GPIO interrupt). Cycle advances
  the index with wrap-around and refreshes the OLED; send calls
  `radio.transmit()` with the selected message.
- **Receive**: DIO0 interrupt sets a flag (or poll for a packet); on reception,
  read the payload as a string, show it on the OLED and print it to Serial.
- **OLED UI**: show selected message + index (e.g. `2/5: On my way`) and the
  last received message.

## Build / flash / monitor

- `pio run` — build
- `pio run -t upload` — flash (same firmware to both devices)
- `pio device monitor` — Serial monitor at 115200 baud
- In VS Code use the PlatformIO IDE extension (see `.vscode/extensions.json`).

## Git hygiene

- `.gitignore` is currently **empty**, so `.pio/` build artifacts got committed.
  Do not add or modify anything under `.pio/`. When touching `.gitignore`, add
  `.pio/` (and optionally `.vscode/`) to it.
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
