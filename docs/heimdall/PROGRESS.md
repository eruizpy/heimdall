# Heimdall — Progreso y memoria del proyecto

Memoria compartida entre equipos (Ubuntu / MacBook). Actualizar al final de cada sesión. Lo más reciente va arriba.

## Estado actual

- **Etapa:** la placa `esp32-2432s028` (CYD2USB) compila; falta probarla en el hardware.
- **Próximo paso:** conectar el INMP441, grabar el firmware y verificar la pantalla (orientación y colores), el micrófono y el parlante.
- **Compilar:** `. ~/esp/esp-idf-v6.1/export.sh && python3 scripts/build.py esp32-2432s028 --name esp32-2432s028`

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
