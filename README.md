# LoRa Offline Communication System

Offline, peer-to-peer messaging between **two Heltec WiFi LoRa 32 boards** over
LoRa (SX1262) — no WiFi, internet, or broker needed. Each device is preloaded
with 10 short messages; its buttons cycle through them on the OLED and send the
selected one over LoRa, so it can appear on the other device when in range.

## Hardware

- 2x Heltec WiFi LoRa 32 boards (ESP32-S3 + SX1262 LoRa + 128x64 SSD1306 OLED)
- **Attach the LoRa antenna before powering on** — never transmit without it
- USB-C cable (power + flashing)
- One momentary push button wired between **GPIO 4** and **GND** — the "prev"
  button. The other button is the onboard **PRG** button (GPIO 0).

## Getting started on your own laptop/PC

1. Install [VS Code](https://code.visualstudio.com/) + the **PlatformIO IDE**
   extension (see `.vscode/extensions.json`), or install PlatformIO Core for the
   command line: `pip install platformio`
2. Install the USB-serial driver if your OS does not recognize the board
   (CP210x or CH340/CH343 depending on board revision — check Device Manager on
   Windows, `ls /dev/tty*` on Linux/macOS)
3. Clone this repository and open the project folder in VS Code (or just work
   from the repo root with the `pio` CLI)

## Build, flash and monitor over USB

With the board plugged in via USB:

```bash
pio run                 # build the firmware
pio device list         # list serial ports; note the one that appears when the board is plugged in
pio run -t upload       # flash the firmware over USB — the SAME firmware goes on BOTH devices
pio device monitor      # Serial monitor at 115200 baud
```

In VS Code + PlatformIO IDE, use the status-bar buttons instead: **Build** (✓),
**Upload** (→), **Serial Monitor**.

If the wrong port is auto-selected, set `upload_port` in `platformio.ini`
(e.g. `COM5` on Windows, `/dev/ttyUSB0` on Linux). With two boards connected,
flash them one at a time: `pio run -t upload --upload-port COM5`.

> The project targets `env:heltec_wifi_lora_32_V3` (`board = heltec_wifi_lora_32_V3`).
> If your physical boards are V4 with a different board id or pinout, update
> `board` in `platformio.ini` and the pin constants at the top of `src/main.cpp`.

## Using the application

1. Plug a board into your laptop via USB — this powers it — and flash:
   `pio run -t upload`.
2. The OLED lights up showing `1/10` with `RUN`. Open `pio device monitor` to
   watch the logs.
3. **Short press PRG** (GPIO 0) → next message. **Press the GPIO 4 button** →
   previous message. The list wraps around in both directions, and each change
   is printed to Serial (`Selected 3/10: BEHIND YOU`).
4. **Hold PRG for about 1 second** → sends the selected message over LoRa.
   The bottom line of the OLED shows `Sending...` then `Sent!` (or
   `Send error N`), and Serial prints `Sent: <message>`.
5. Radio settings (constants `LORA_*` at the top of `src/main.cpp`, must be
   identical on both devices): **868 MHz** (EU — change `LORA_FREQ_HZ` for
   other regions), SF7, BW 125 kHz, CR 4/5, 10 dBm TX power, private sync word.
   Expect a few hundred meters line-of-sight with the stock antenna.
6. **Receiving is not implemented yet.** For now, verify sending via the OLED
   status line and the `Sent: ...` line in the Serial monitor. Once receive
   lands, the other device's OLED will show the incoming message.

## Project layout

- `src/main.cpp` — firmware: pin config, `MESSAGES[]` table, button handling, OLED UI, SX1262 radio
- `platformio.ini` — build env and libraries (RadioLib for the SX1262, U8g2 for the OLED)
- `AGENTS.md` / `CLAUDE.md` — project documentation and agent instructions (kept in sync)

## Troubleshooting

- **No serial port appears:** try another USB cable (some are charge-only),
  install the USB-serial driver, or check Device Manager.
- **OLED stays blank:** the OLED is powered via Vext (GPIO 36, active LOW) —
  check the `OLED_*` pins at the top of `src/main.cpp`.
- **`Radio error N` at boot** (OLED status line / Serial): SX1262 init failed —
  check the `LORA_*` pins and that the antenna is attached.
- **Build fails after pulling changes:** delete the `.pio/` folder (it is
  gitignored) and run `pio run` again.
