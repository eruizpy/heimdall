# 002 — Plan técnico

## Diseño

| Módulo | Tipo | Responsabilidad |
|---|---|---|
| `face/main/face_proto.c` | Puro | Interpretar una línea JSON en un comando validado; formatear eventos de touch |
| `face/main/face_expr.c` | Puro | Tabla de expresiones y búsqueda por nombre de emoción |
| `face/main/touch_calib.c` | Puro | Calcular la calibración de 3 puntos y convertir lecturas crudas a pantalla |
| `face/main/face_ui.c` | Hardware (LVGL) | Dibujar ojos y textos, animaciones, gestos |
| `face/main/touch.c` | Hardware | XPT2046 por SPI3, indev de LVGL, NVS, tarea de calibración |
| `face/main/link.c` | Hardware | UART0 y UART2, lectura de líneas, despacho de comandos |
| `face/main/display.c` | Hardware | ST7789, retroiluminación, puerto de LVGL |

El contrato con el core es [face-protocol.md](../../heimdall/face-protocol.md).

## Decisiones

| Decisión | Alternativas descartadas | Motivo |
|---|---|---|
| Ojos dibujados con rectángulos y triángulos | Rotación de objetos LVGL | Las capas de transformación necesitan ~100 KB; no hay PSRAM |
| JSON por línea a 115200 | Protocolo binario | Depurable a mano desde la computadora; ancho de banda sobrado |
| Proyecto ESP-IDF separado (`face/`) | Variante del firmware principal | La cara no necesita WiFi, audio ni la aplicación completa |
| Calibración de 3 puntos con detección de ejes | Valores fijos | Las CYD varían; esta tiene ejes cruzados |

## Riesgos

- Ruido del touch: mediana de 5 lecturas y umbral de presión.
- Logs mezclados con el protocolo en el USB: los logs bajan a nivel aviso y las líneas del protocolo empiezan con `{`.

## Estrategia de pruebas

| Nivel | Qué se prueba |
|---|---|
| Unitario (host) | Protocolo, expresiones, calibración |
| Unitario (Python) | Simulador del core |
| HIL | ping/pong, comandos y eventos con la placa |
| Manual | Aspecto de los ojos y precisión del toque |
