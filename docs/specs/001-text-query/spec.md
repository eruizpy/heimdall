# 001 — Consultas de texto por puerto serie

| Campo | Valor |
|---|---|
| Estado | Verificada (retroactiva) |
| Autor | eruizpy |
| Fecha | 2026-11-09 |

## Problema

Sin micrófono no hay forma de probar el asistente de punta a punta. Hace falta enviarle preguntas escritas y que responda por el dispositivo (pantalla y parlante) igual que si se le hubiera hablado.

## Alcance

- Incluye: recibir líneas por el UART0 (USB) de la placa CYD, enviarlas al servidor como consulta, mostrar y reproducir la respuesta; script de la computadora para preguntar y ver la respuesta.
- No incluye: autenticación del puerto serie (es acceso físico), consultas mientras el dispositivo habla.

## Requisitos funcionales

- **RF1** — Cada línea recibida por el puerto serie se envía al servidor como una consulta del usuario.
- **RF2** — La consulta usa el mensaje existente `listen`/`detect` con texto; el protocolo con el servidor no cambia.
- **RF3** — Tras la respuesta, el dispositivo vuelve a reposo sin transmitir audio del micrófono.
- **RF4** — Si el dispositivo no está en reposo, la consulta se descarta con un aviso en el log.
- **RF5** — `scripts/heimdall_ask.py "pregunta"` envía la consulta y muestra lo que entendió el servidor (`>>`) y la respuesta (`<<`).

## Requisitos no funcionales

- **RNF1** — El texto enviado no puede romper el JSON del protocolo (comillas, barras, caracteres de control) y conserva acentos y ñ (UTF-8).
- **RNF2** — Líneas de hasta 200 caracteres; las más largas se truncan.
- **RNF3** — Conexión al servidor en ≤ 3 s desde reposo; primera frase de respuesta en ≤ 4 s.

## Criterios de aceptación

- **CA1** — Dado un texto con comillas, barras y saltos de línea, cuando se sanea, entonces se eliminan esos caracteres y se conservan los acentos. — *Verificación:* `tests/host/test_text_query.cc`.
- **CA2** — Dado el dispositivo en reposo, cuando se ejecuta `heimdall_ask.py "Decí solo: hola"`, entonces el log muestra `>>` con la consulta y `<<` con la respuesta. — *Verificación:* HIL (registrado en PROGRESS.md, 2026-11-09).
- **CA3** — Dada una pregunta de control ("bajá el brillo al 40 %"), entonces el dispositivo ejecuta la herramienta MCP correspondiente. — *Verificación:* HIL manual (2026-11-09).
- **CA4** — Desde reposo, la conexión WebSocket tarda ≤ 3 s. — *Verificación:* HIL, medido 1,8 s (2026-11-09).
- **CA5** — `heimdall_ask.py` reconoce las líneas relevantes del log y descarta el resto. — *Verificación:* `scripts/tests/test_heimdall.py`.

## Historial

| Fecha | Cambio |
|---|---|
| 2026-11-09 | Especificación retroactiva |
