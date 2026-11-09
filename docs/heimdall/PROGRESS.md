# Heimdall — Progreso y memoria del proyecto

Memoria compartida entre equipos (Ubuntu / MacBook). Actualizar al final de cada sesión. Lo más reciente va arriba.

## Estado actual

- **Etapa:** Heimdall funciona de punta a punta con el servidor propio: la placa está activada en `<dominio>`, conecta por `wss://` y el servidor carga Groq + EdgeTTS. Falta el micrófono para hablarle.
- **Servidor propio:** desplegado en `<dominio>` (ver `deploy/vps/README.md`); el firmware ya apunta a `https://<dominio>/xiaozhi/ota/`.
- **Probar sin micrófono:** `python3 scripts/heimdall_ask.py "pregunta"` manda texto por el puerto serie y muestra la respuesta (con el entorno de ESP-IDF cargado).
- **Próximo paso:** comprar e instalar el INMP441 y probar una conversación real.
- **Cara del robot (etapa 1, hecha):** firmware `face/` en la CYD: ojos animados, textos, touch calibrado y protocolo UART (`docs/heimdall/face-protocol.md`). Probar con `python3 scripts/heimdall_face.py demo`. La CYD hoy tiene este firmware, no el Heimdall autónomo.
- **Robot (plan):** CYD como cara táctil + ESP32-S3 como core (voz, cabeza pan-tilt, hexápodo de 18 servos). Arquitectura y compras en `docs/heimdall/compras.md`: compra A (voz y cabeza) en noviembre 2026, compra B (hexápodo) en diciembre 2026.
- **Compilar:** `. ~/esp/esp-idf-v6.1/export.sh && python3 scripts/build.py esp32-2432s028 --name esp32-2432s028 --language es-ES`

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
python3 scripts/build.py esp32-2432s028 --name esp32-2432s028 --language es-ES
ls /dev/cu.*                      # puerto: /dev/cu.usbserial-* o /dev/cu.wchusbserial-*
idf.py -p /dev/cu.usbserial-XXXX flash monitor   # salir con Ctrl+]
```

Qué verificar:
- **Pantalla:** confirmada el 2026-11-09 (`DISPLAY_MIRROR_X false`, `DISPLAY_INVERT_COLOR false`).
- **WiFi:** la placa crea la red `Heimdall-XXXX`; conectarse a ella y configurar el WiFi de casa.
- **Servidor:** la pantalla muestra un código de 6 dígitos; activar el dispositivo en xiaozhi.me (ver "Servidor" abajo).
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
| 2026-11-06 | Idioma del dispositivo: español (`--language es-ES` al compilar; no va en el config.json de la placa). |
| 2026-11-06 | Servidor: el oficial xiaozhi.me por ahora; más adelante, uno propio (la URL ya es configurable). |
| 2026-11-06 | Red WiFi de configuración y hostname: `Heimdall-XXXX` (`boards/common/wifi_board.cc`, afecta a todas las placas). |
| 2026-11-09 | Táctil: solo se usa la interrupción PENIRQ del XPT2046 (GPIO36); tocar la pantalla equivale al botón BOOT. No se leen coordenadas (sin SPI ni calibración). |
| 2026-11-09 | Sin palabra de activación en la CYD: en ESP32 clásico la wake word de xiaozhi requiere PSRAM (`USE_ESP_WAKE_WORD` depende de `SPIRAM`) y la placa no tiene. |
| 2026-11-09 | Servidor propio: xiaozhi-esp32-server en modo completo con Docker, en un Linode 2 GB (`<ip-vps>`) con dominio `<dominio>` y Caddy (HTTPS automático). Archivos en `deploy/vps/`. |
| 2026-11-09 | IA: Groq (Whisper `whisper-large-v3-turbo` + LLM `openai/gpt-oss-120b`) + EdgeTTS voz `es-MX-JorgeNeural`. La cuenta de Groq no tiene Llama; gpt-oss-120b respondió en ~0,5 s. Sin modelos locales (no entran en 2 GB). |
| 2026-11-09 | OpenAI como LLM descartado por ahora: el proveedor `openai` del servidor no puede enviar `reasoning_effort: none` (requerido para funciones en Chat Completions con los modelos GPT-6). Haría falta un parche propio. |
| 2026-11-09 | La placa usa `CONFIG_OTA_URL=https://<dominio>/xiaozhi/ota/` (en `config.json`); xiaozhi.me ya no se usa. |
| 2026-11-09 | Caddy usa certificados RSA-2048 (`key_type rsa2048`): la ESP32 acelera RSA por hardware pero verifica la cadena ECDSA P-384 de Let's Encrypt por software (~3 s por conexión TLS). |
| 2026-11-09 | Robot: la CYD pasa a ser la cara (pantalla + touch con coordenadas) conectada por UART (CN1) a un core ESP32-S3 N16R8 con voz, cabeza pan-tilt (2 servos por LEDC) y hexápodo (18 MG90S por 2× PCA9685). La CYD standalone actual se conserva como variante. |
| 2026-11-09 | Si se agrega el MAX98357A, va como variante/placa nueva (no cambiar pines de una placa existente: afecta OTA). Pines libres escasos: definir el pin de datos antes de comprar. |

## Servidor

**Fase actual (desde 2026-11-09): servidor propio en `https://<dominio>`**, ver `deploy/vps/README.md`. Lo de abajo sobre xiaozhi.me queda como referencia.

**Fase anterior: servidor oficial xiaozhi.me** (la URL por defecto de xiaozhi, `https://api.tenclass.net/xiaozhi/ota/`).

1. Crear una cuenta en https://xiaozhi.me y entrar a la consola.
2. Crear un agente con nombre **Heimdall**, idioma español, una voz en español, y escribir su personalidad (rol/prompt).
3. Encender la placa con WiFi: la pantalla muestra un código de 6 dígitos. Agregar el dispositivo en la consola con ese código y asignarle el agente.

El nombre al hablar, la personalidad, la voz y el modelo de IA se configuran en la consola, no en el firmware.

**Fase futura: servidor propio** (por ejemplo xiaozhi-esp32-server, en una PC o un VPS). La URL ya es configurable sin cambiar código:
- al compilar: `CONFIG_OTA_URL` (menuconfig → Xiaozhi Assistant → Default OTA URL);
- sin recompilar: el firmware usa primero la clave NVS `wifi/ota_url` si está guardada.

## Configuración por equipo

| Equipo | ESP-IDF | Puerto serie | Notas |
|---|---|---|---|
| Ubuntu | v6.1.0 en `~/esp/esp-idf-v6.1` (cmake/ninja vía `idf_tools.py install cmake ninja`) | — | Identidad git local: Esteban RD <eruizpy@gmail.com>. Para grabar falta el grupo `dialout`. |
| MacBook | v6.1 en `~/esp/esp-idf-v6.1` (instalado 2026-11-09); `ninja`, `dfu-util`, `clang-format` vía Homebrew | `/dev/cu.usbserial-10` (CH340, driver de Apple) | Disco casi lleno (~18 GB libres). Tests de host: 147 OK. |

## Pendientes

- [x] Elegir placa/hardware objetivo.
- [x] Instalar ESP-IDF v6.1 en la MacBook.
- [x] Probar en hardware: pantalla, WiFi y conexión al servidor (2026-11-09).
- [x] Probar en hardware: táctil y BOOT (2026-11-09).
- [ ] Probar en hardware: micrófono y parlante.
- [ ] Mejor audio: amplificador I2S MAX98357A + parlante de 4–8 Ω/3 W (en evaluación, ver bitácora).
- [ ] Batería: LiPo 1S + módulo de carga USB-C con salida de 5 V (en evaluación, ver bitácora).
- [x] Táctil XPT2046: tocar = botón BOOT (confirmado en hardware el 2026-11-09).
- [ ] ~~Palabra de activación~~: no es posible en la CYD (requiere PSRAM); se usa el botón BOOT o el táctil.
- [x] Idioma (es-ES) y servidor (xiaozhi.me por ahora).
- [x] Consola de <dominio> configurada y placa vinculada al agente Heimdall (2026-11-09).
- [x] Consultas de texto por puerto serie (`scripts/heimdall_ask.py`): respuesta en ~2 s, control de brillo por voz OK (2026-11-09).
- [ ] Probar una conversación completa cuando llegue el micrófono.
- [x] Conexión lenta de la placa (~10 s): corregida, ahora ~1,8 s (2026-11-09).
- [ ] Evaluar `AUDIO_OUTPUT_SAMPLE_RATE 16000` en la placa: el servidor envía 16 kHz y hoy se remuestrea a 24 kHz.
- [x] Sonidos es-ES revisados (transcritos con Whisper): ninguno dice "Xiaozhi"; `welcome.ogg` ("¡Hola, mi amigo!") no se usa en el código.
- [x] Servidor propio en un VPS: desplegado (2026-11-09).
- [x] Respaldos automáticos del VPS: diarios a las 03:30, 14 días, restauración probada (2026-11-09).
- [x] Copia de los respaldos fuera del VPS: `deploy/vps/pull-backups.sh` → `~/heimdall-backups/` (correr cada tanto; primera copia en la MacBook el 2026-11-09).
- [x] Instalar clang-format (MacBook, Homebrew).

## Bitácora

### 2026-11-09 — MacBook (cara del robot)
- Nuevo proyecto ESP-IDF `face/` para la CYD: ST7789 + LVGL 9.5, ojos dibujados a mano (sin transformaciones: no hay PSRAM), 10 expresiones con nombres de xiaozhi, parpadeo y miradas automáticas, ojos que siguen el dedo.
- Touch XPT2046 en SPI3 con calibración de 3 puntos guardada en NVS; eventos tap, long y swipe.
- Enlace UART: USB (desarrollo) y CN1 GPIO27 TX / GPIO22 RX (core), JSON por línea a 115200.
- `scripts/heimdall_face.py` simula al core (monitor, send, demo). Probado en la placa: hello, ping, expresiones y todos los eventos (tap, long, swipe en las 4 direcciones). La calibración detectó ejes cruzados (raw Y ↔ pantalla X) y los corrige.

### 2026-11-09 — MacBook (latencia de conexión)
- Diagnóstico con tcpdump en el VPS: sin pérdida de SYN ni problema de DNS. Dos causas: (1) `SendTextQuery` conectaba con el WiFi en `MAX_MODEM` (cada ida y vuelta esperaba el intervalo de escucha de ~1 s); ahora pasa a `PERFORMANCE` antes, como la palabra de activación. (2) ~3,4 s de criptografía en la ESP32 verificando la cadena ECDSA.
- Caddy pasó a RSA-2048; los certificados ECDSA viejos quedaron en `/opt/heimdall/caddy/old-ecdsa-certs/`.
- Resultado: WebSocket 10,3 s → 1,8 s; OTA al arrancar 6,2 s → 2,5 s; pregunta en frío → primera frase en 3,8 s.

### 2026-11-09 — MacBook (consultas de texto)
- Nuevo `Application::SendTextQuery()` (núcleo, genérico): abre el canal y manda el texto con `listen`/`detect`, modo manual para no transmitir el micrófono. La placa lee líneas del UART0 y las envía.
- Bug encontrado: `uart_read_bytes` con `portMAX_DELAY` espera a llenar el búfer; se lee 1 byte bloqueante y luego lo disponible.
- Pruebas: "¿quién sos?", "bajá el brillo al 40 %" (usó la herramienta del dispositivo) y un dato sobre Paraguay. Primera frase ~2 s después de la consulta.

### 2026-11-09 — MacBook (respaldos)
- `deploy/vps/backup.sh` + `heimdall-backup.timer`: dump de MySQL y configuración/secretos en `/opt/heimdall/backups/`. Restauración probada en una base temporal: tablas y acentos coinciden.
- Zona horaria del VPS cambiada a America/Asuncion.
- `deploy/vps/pull-backups.sh` copia los respaldos a `~/heimdall-backups/` del equipo local (rsync, sin borrar copias viejas).

### 2026-11-09 — MacBook (consola y activación)
- Admin registrado (el registro público quedó cerrado). La contraseña exige mayúsculas, minúsculas, números y un carácter especial.
- Por SQL: LLM `LLM_GroqLLM` (clave copiada de GroqASR), predeterminados Groq, 4 voces EdgeTTS en español (`ai_tts_voice`, insertar con `--default-character-set=utf8mb4`), plantilla de agente Heimdall en español, `server.fronted_url = https://<dominio>`. Tras cambios en la base: borrar en Redis `model:data:*`, `server:config` y `sys:params`.
- Groq bloquea el User-Agent por defecto de Python urllib (403); usar uno propio en pruebas.
- Firmware nuevo grabado: activación OK, conexión `wss://<dominio>/xiaozhi/v1/` OK, 4 herramientas MCP de la placa registradas. Servidor de voz ~200 MB de RAM con Groq.

### 2026-11-09 — MacBook (VPS)
- Linode redimensionado a 2 GB; se agregaron 2 GB de swap, Docker (repo oficial) y `ufw` (22, 80, 443).
- Desplegado xiaozhi-esp32-server 0.9.8 (modo completo) en `/opt/heimdall`; imágenes desde `ghcr.io` (el compose oficial usa un espejo chino). MySQL 8.4 y Java con memoria limitada.
- Caddy obtuvo los certificados de `<dominio>` y `www`. La contraseña de MySQL y el `server.secret` solo están en el VPS.
- Memoria en uso: ~660 MB sin el servidor de voz; FunASR (ASR local por defecto) pide 2 GB: hay que cambiar a Groq.

### 2026-11-09 — MacBook (primera prueba en hardware)
- Grabada la CYD2USB (ESP32-D0WD-V3). Arranque limpio, ~128 KB de RAM libre con WiFi; red `Heimdall-42F9`.
- Pantalla: venía espejada en horizontal y con colores invertidos; corregido en `config.h`.
- WiFi de casa configurado desde el portal; OTA en api.tenclass.net responde; queda esperando activación.
- macOS: la placa no crea `/dev/cu.usbserial-*` hasta permitir el accesorio USB (Privacidad y seguridad) y reconectarla.
- `idf.py flash` falla tras `scripts/build.py` por diferencia `python`/`python3` en el entorno; grabar desde `build/` con `python -m esptool --chip esp32 -p <puerto> -b 460800 --before default-reset --after hard-reset write-flash "@flash_args"`.

### 2026-11-09 — MacBook
- Revisión del proyecto. Instalados ESP-IDF v6.1, ninja, dfu-util y clang-format.
- Táctil: PENIRQ del XPT2046 como segundo botón (tocar = BOOT).
- Audios es-ES transcritos: ninguno menciona "Xiaozhi".
- Tests de host en la Mac: 147 OK (los 6 fallos de `test_ci_selection` son solo del Ubuntu).
- Build `esp32-2432s028` (es-ES) en la Mac con IDF v6.1.0: OK, 1,9 MB (35 % libre).

### 2026-11-06 — Ubuntu (personalización)
- Red WiFi de configuración/hostname cambiada a `Heimdall-XXXX`.
- Se compila con `--language es-ES` (compila bien; el firmware ocupa 1,9 MB, con 35 % libre).
- Se documentaron la activación en xiaozhi.me y cómo migrar a un servidor propio.

### 2026-11-05 — Ubuntu (placa)
- Se creó la placa `esp32-2432s028`: pantalla ST7789 320x240 en HSPI, retroiluminación en GPIO21, botón BOOT, LED RGB apagado.
- El build compila; todavía no se probó en hardware.
- Tests de host: los 6 errores de `test_ci_selection` ya estaban antes de este cambio.
- Hardware recomendado (pendiente de decidir): MAX98357A con datos de salida en un pin extra (I2S dúplex compartiendo BCLK/WS con el micrófono). Batería LiPo + módulo de carga y elevación a 5 V (IP5306 o TP4056+MT3608) con USB-C, conectado a VIN del conector P1.

### 2026-11-05 — Ubuntu
- Se clonó heimdall y se importó xiaozhi-esp32 con su historial completo.
- Se creó `CLAUDE.md` con la regla de continuidad entre máquinas y este archivo de progreso.
