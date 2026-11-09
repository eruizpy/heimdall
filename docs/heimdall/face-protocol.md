# Protocolo cara ↔ core

La cara (CYD ESP32-2432S028, firmware `face/`) y el core (ESP32-S3) hablan por UART con **una línea JSON por mensaje** (UTF-8, terminada en `\n`, máximo 256 bytes).

| Enlace | Pines en la CYD | Velocidad |
|---|---|---|
| Core (producción) | CN1: TX = GPIO27, RX = GPIO22, GND, 3.3V (5 V por P1/VIN) | 115200 8N1 |
| Mac por USB (desarrollo) | UART0 del puerto USB | 115200 8N1 |

La cara escucha y responde por los dos enlaces a la vez. En el USB también salen logs; las líneas del protocolo son las que empiezan con `{`.

## Core → cara

| Mensaje | Efecto |
|---|---|
| `{"t":"emo","v":"happy"}` | Expresión de los ojos. Acepta los nombres de emoción de xiaozhi (`neutral`, `happy`, `laughing`, `sad`, `crying`, `angry`, `surprised`, `shocked`, `thinking`, `sleepy`, `loving`, `winking`, `confused`, …); los desconocidos van a `neutral`. |
| `{"t":"status","v":"Escuchando"}` | Texto chico arriba. |
| `{"t":"text","role":"assistant","v":"Hola"}` | Texto grande abajo. `role`: `assistant` (blanco), `user` (gris), `system` (celeste). `v` vacío lo borra. |
| `{"t":"look","x":-0.5,"y":0.2}` | Hacia dónde miran los ojos, de -1 a 1 (para acompañar a la cabeza). Las miradas automáticas se pausan 3 s. |
| `{"t":"blink"}` | Parpadeo. |
| `{"t":"bright","v":80}` | Brillo de la pantalla, 0–100. |
| `{"t":"calibrate"}` | Calibración del touch (3 cruces en pantalla). |
| `{"t":"ping"}` | La cara responde `{"t":"pong"}`. |

## Cara → core

| Mensaje | Cuándo |
|---|---|
| `{"t":"hello","fw":"heimdall-face 0.1"}` | Al arrancar. |
| `{"t":"touch","ev":"tap","x":160,"y":120,"raw":[2048,2048]}` | Toque corto. `ev` también puede ser `long`, `swipe_left`, `swipe_right`, `swipe_up`, `swipe_down`. Coordenadas de pantalla (320×240); `raw` es la lectura del XPT2046. |
| `{"t":"calibrated"}` | Terminó la calibración (se guarda en NVS). |
| `{"t":"pong"}` | Respuesta a `ping`. |

Si la cara no tiene calibración guardada, la pide sola al arrancar.
