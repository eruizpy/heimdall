# 002 — Cara del robot en la CYD

| Campo | Valor |
|---|---|
| Estado | Verificada (retroactiva) |
| Autor | eruizpy |
| Fecha | 2026-11-09 |

## Problema

El robot necesita una cara expresiva y una entrada táctil. La CYD (ESP32-2432S028) tiene pantalla y touch pero pocos pines y sin PSRAM, así que se dedica solo a eso y la controla el core del robot por UART.

## Alcance

- Incluye: ojos animados con expresiones, textos de estado y conversación, touch con calibración y gestos, protocolo JSON por UART (USB y conector CN1), script que simula al core.
- No incluye: audio, WiFi, lógica de conversación (son del core).

## Requisitos funcionales

- **RF1** — Al arrancar envía `{"t":"hello","fw":...}`; responde `pong` a `ping`.
- **RF2** — Acepta los comandos `emo`, `status`, `text`, `look`, `blink`, `bright`, `calibrate` definidos en [face-protocol.md](../../heimdall/face-protocol.md).
- **RF3** — Mapea los nombres de emoción del core a 10 expresiones; un nombre desconocido, vacío o ausente muestra la expresión neutral.
- **RF4** — Detecta toque corto, toque largo y deslizamiento en 4 direcciones y envía `{"t":"touch",...}` con coordenadas de pantalla y lectura cruda.
- **RF5** — Calibra el touch con 3 puntos, detecta ejes cruzados e invertidos y guarda la calibración en NVS; la pide sola si no existe.
- **RF6** — Parpadea y mira alrededor sola; sigue el dedo al tocar.

## Requisitos no funcionales

- **RNF1** — Sin PSRAM: nada de capas de transformación de LVGL; memoria interna suficiente con margen.
- **RNF2** — Una línea inválida, demasiado larga o desconocida se descarta sin bloquear ni reiniciar.
- **RNF3** — Textos con acentos y ñ.
- **RNF4** — Coordenadas siempre dentro de 320×240.

## Criterios de aceptación

- **CA1** — Cada comando del protocolo se interpreta con sus campos; JSON inválido, sin `t` o con `t` desconocido se rechaza. — *Verificación:* `tests/host/test_face_proto.c`.
- **CA2** — Valores fuera de rango (`look` fuera de ±1, `bright` fuera de 0–100) se acotan; textos largos se truncan sin desbordar. — *Verificación:* `tests/host/test_face_proto.c`.
- **CA3** — Los nombres de emoción se resuelven por palabra completa; nombre vacío o nulo da neutral y termina. — *Verificación:* `tests/host/test_face_expr.c`.
- **CA4** — La calibración resuelve ejes normales, cruzados e invertidos, rechaza puntos degenerados y acota las coordenadas. — *Verificación:* `tests/host/test_touch_calib.c`.
- **CA5** — Con la placa conectada, `ping` → `pong` y los eventos de touch llegan con coordenadas coherentes. — *Verificación:* HIL con `heimdall_face.py` (2026-11-09: 14 tap, 1 long, swipes en 4 direcciones).
- **CA6** — `heimdall_face.py` arma los mensajes del protocolo y reacciona a los eventos. — *Verificación:* `scripts/tests/test_heimdall.py`.

## Historial

| Fecha | Cambio |
|---|---|
| 2026-11-09 | Especificación retroactiva. Al escribir los tests de CA3 apareció un bucle infinito con nombre de emoción vacío: corregido. |
