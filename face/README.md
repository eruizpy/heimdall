# Heimdall face (CYD)

Firmware for the ESP32-2432S028 (CYD2USB) as the robot's face: animated eyes, status/text and
touch input. It does not use WiFi or audio; the robot core (ESP32-S3) drives it over UART.
Protocol: [docs/heimdall/face-protocol.md](../docs/heimdall/face-protocol.md).

This is a separate ESP-IDF project (not built by `scripts/build.py`). The standalone Heimdall
firmware for the same board is still `main/boards/esp32-2432s028`.

## Build and flash

```sh
. ~/esp/esp-idf-v6.1/export.sh
cd face
idf.py set-target esp32   # first time only
idf.py -p /dev/cu.usbserial-XXXX flash
```

On first boot (no calibration in NVS) it asks to tap 3 crosses. Re-run it with
`python3 scripts/heimdall_face.py send calibrate`.

## Try it from the computer (acts as the core)

```sh
python3 scripts/heimdall_face.py demo                 # cycles expressions, reacts to touches
python3 scripts/heimdall_face.py monitor              # prints touch events
python3 scripts/heimdall_face.py send emo angry
python3 scripts/heimdall_face.py send text "Hola, soy Heimdall"
```

## Wiring to the core

| CYD | Core ESP32-S3 |
|---|---|
| CN1 GPIO27 (TX) | UART RX |
| CN1 GPIO22 (RX) | UART TX |
| CN1 GND | GND |
| P1 VIN (5 V) | 5 V supply |

## Notes

- Eyes are drawn with rectangles and triangles in a draw event: LVGL rotation transforms would need
  ~100 KB of RAM and the board has no PSRAM.
- XPT2046 runs on its own SPI bus (SPI3) at 1 MHz; the display uses SPI2.
- Fonts: Noto Sans from `78/xiaozhi-fonts` (covers Spanish accents and ñ).
