# Heimdall — Progreso y memoria del proyecto

Memoria compartida entre equipos (Ubuntu / MacBook). Actualizar al final de cada sesión. Lo más reciente va arriba.

## Estado actual

- **Etapa:** la placa `esp32-2432s028` (CYD2USB) compila; falta probarla en el hardware.
- **Próximo paso:** avanzar sin el hardware de audio ni la batería (todavía no comprados): probar la pantalla con la placa sola, el táctil y la personalización.
- **Por comprar (Amazon):** micrófono INMP441, amplificador MAX98357A + parlante de 4–8 Ω/3 W, batería LiPo 1S 1000–2000 mAh + módulo de carga USB-C con salida de 5 V (IP5306 o TP4056+MT3608).
- **Compilar:** `. ~/esp/esp-idf-v6.1/export.sh && python3 scripts/build.py esp32-2432s028 --name esp32-2432s028`

## Retomar en la MacBook

Próxima sesión: probar la placa (solo la pantalla, sin micrófono ni parlante) desde la MacBook.

```sh
# Una sola vez: dependencias y ESP-IDF v6.1
xcode-select --install            # si falta
brew install cmake ninja dfu-util python3
mkdir -p ~/esp && cd ~/esp
git clone -b release/v6.1 --recursive --depth 1 --shallow-submodules https://github.com/espressif/esp-idf.git esp-idf-v6.1
cd esp-idf-v6.1 && ./install.sh esp32

# Cada sesión
cd ~/ruta/a/heimdall && git pull --ff-only origin main
. ~/esp/esp-idf-v6.1/export.sh
python3 scripts/build.py esp32-2432s028 --name esp32-2432s028
ls /dev/cu.*                      # puerto: /dev/cu.usbserial-* o /dev/cu.wchusbserial-*
idf.py -p /dev/cu.usbserial-XXXX flash monitor   # salir con Ctrl+]
```

Qué verificar:
- **Pantalla:** orientación, espejado y colores. Si algo falla, ajustar `DISPLAY_MIRROR_X/Y`, `DISPLAY_INVERT_COLOR` o `DISPLAY_RGB_ORDER` en `main/boards/esp32-2432s028/config.h`.
- **WiFi:** el portal de configuración y la conexión.
- **Botón BOOT:** que cambie el estado del chat.
- **Monitor serie:** que no haya errores de memoria ni reinicios.

Si macOS no detecta la placa: puede faltar el driver CH340/CH341 (en macOS recientes suele venir incluido). Probar otro cable USB-C que transmita datos.

## Decisiones

| Fecha | Decisión |
|---|---|
| 2026-11-05 | Heimdall parte de xiaozhi-esp32 (`main` @ `a73e9a7`), con el historial completo. |
| 2026-11-05 | Se trabaja directo en `main`, con commit y push tras cada cambio, para poder seguir desde cualquier equipo. |
| 2026-11-05 | LICENSE MIT: se conservan los avisos originales y se agrega "Esteban RD". |
| 2026-11-05 | Placa: ESP32-2432S028 **CYD2USB** (micro-USB + USB-C, pantalla ST7789). Nueva placa en `main/boards/esp32-2432s028/`. |
| 2026-11-05 | Audio v1: micrófono INMP441 (SCK 22, WS 27, SD 35) en I2S1 + parlante de la placa vía DAC interno GPIO26 (driver propio `DacAudioCodec`). |
| 2026-11-05 | `esp_driver_dac` se agrega según `IDF_TARGET` (esp32): los `CONFIG_*` no existen en la early expansion de CMake. |

## Configuración por equipo

| Equipo | ESP-IDF | Puerto serie | Notas |
|---|---|---|---|
| Ubuntu | v6.1.0 en `~/esp/esp-idf-v6.1` (cmake/ninja vía `idf_tools.py install cmake ninja`) | — | Identidad git local: Esteban RD <eruizpy@gmail.com>. Para grabar falta el grupo `dialout`. |
| MacBook | sin verificar | — | — |

## Pendientes

- [x] Elegir placa/hardware objetivo.
- [ ] Instalar ESP-IDF v6.1 en la MacBook.
- [ ] Probar en hardware: pantalla (mirror/invert/RGB en `config.h`), micrófono y parlante.
- [ ] Mejor audio: amplificador I2S MAX98357A + parlante de 4–8 Ω/3 W (en evaluación, ver bitácora).
- [ ] Batería: LiPo 1S + módulo de carga USB-C con salida de 5 V (en evaluación, ver bitácora).
- [ ] Táctil XPT2046 (CLK 25, MOSI 32, MISO 39, CS 33, IRQ 36).
- [ ] Palabra de activación (desactivada para el ESP32 clásico; hoy se usa el botón BOOT).
- [ ] Definir el nombre/identidad del asistente, el servidor y el idioma (el build usa en-US por defecto).
- [ ] Instalar clang-format para formatear el código modificado.

## Bitácora

### 2026-11-05 — Ubuntu (placa)
- Se creó la placa `esp32-2432s028`: pantalla ST7789 320x240 en HSPI, retroiluminación en GPIO21, botón BOOT, LED RGB apagado.
- El build compila; todavía no se probó en hardware.
- Tests de host: los 6 errores de `test_ci_selection` ya estaban antes de este cambio.
- Hardware recomendado (pendiente de decidir): MAX98357A con datos de salida en un pin extra (I2S dúplex compartiendo BCLK/WS con el micrófono). Batería LiPo + módulo de carga y elevación a 5 V (IP5306 o TP4056+MT3608) con USB-C, conectado a VIN del conector P1.

### 2026-11-05 — Ubuntu
- Se clonó heimdall y se importó xiaozhi-esp32 con su historial completo.
- Se creó `CLAUDE.md` con la regla de continuidad entre máquinas y este archivo de progreso.
