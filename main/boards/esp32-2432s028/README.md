# ESP32-2432S028 (CYD2USB)

"Cheap Yellow Display" with micro-USB **and** USB-C: ESP32-WROOM-32 (4MB flash, no PSRAM), 2.8" ST7789 240×320 display, XPT2046 resistive touch, RGB LED and a speaker amplifier on the internal DAC.

The board has **no microphone**: an external INMP441 I2S module is required.

## Wiring

| INMP441 | CYD connector | GPIO |
|---|---|---|
| VDD | CN1 3.3V | — |
| GND | CN1 GND | — |
| SCK | CN1 | 22 |
| WS | CN1 | 27 |
| SD | P3 | 35 |
| L/R | GND | — |

Speaker: connector P4 (onboard amplifier, internal DAC on GPIO26).

## Onboard pins in use

| Function | GPIO |
|---|---|
| Display SPI (HSPI) | SCK 14, MOSI 13, MISO 12, CS 15, DC 2 |
| Backlight | 21 |
| BOOT button (chat / Wi-Fi config) | 0 |
| RGB LED (kept off) | R 4, G 16, B 17 |
| Touch XPT2046 (not used yet) | CLK 25, MOSI 32, MISO 39, CS 33, IRQ 36 |

## Build

```sh
python3 scripts/build.py esp32-2432s028 --name esp32-2432s028 --language es-ES
```

## Notes

- The single-USB board (ILI9341 display) is a different variant and is not supported by this build.
- Display orientation and colors (`DISPLAY_MIRROR_*`, `DISPLAY_INVERT_COLOR`, `DISPLAY_RGB_ORDER` in `config.h`) are pending hardware confirmation.
- Audio output is 8-bit through the internal DAC: expect lower fidelity than an I2S amplifier.
