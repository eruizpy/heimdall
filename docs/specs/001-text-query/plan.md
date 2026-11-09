# 001 — Plan técnico

## Diseño

| Pieza | Responsabilidad |
|---|---|
| `main/text_query.h` | `SanitizeTextQuery()`: lógica pura, sin dependencias de ESP-IDF |
| `Application::SendTextQuery()` | Valida estado, sale del ahorro de energía WiFi, abre el canal y envía `listen`/`detect` en modo de escucha manual |
| Placa `esp32-2432s028` | Tarea que lee líneas del UART0 y llama a `SendTextQuery()` |
| `scripts/heimdall_ask.py` | Envía la línea y filtra el log de la placa |

## Decisiones

| Decisión | Alternativas descartadas | Motivo |
|---|---|---|
| Reusar `listen`/`detect` con texto | Mensaje nuevo en el protocolo | El servidor ya lo trata como consulta; cero cambios del lado servidor |
| Modo de escucha manual | Modo automático | Evita transmitir el ruido del pin del micrófono desconectado |
| Salir de `MAX_MODEM` antes de conectar | Mantener ahorro de energía | Cada ida y vuelta esperaba ~1 s de intervalo de escucha |

## Riesgos

- `uart_read_bytes` con `portMAX_DELAY` espera a llenar el búfer: se lee 1 byte bloqueante y luego lo disponible.

## Estrategia de pruebas

| Nivel | Qué se prueba |
|---|---|
| Unitario (host) | Saneamiento del texto |
| Unitario (Python) | Filtro de líneas de `heimdall_ask.py` |
| HIL | Pregunta y respuesta reales con la placa |
