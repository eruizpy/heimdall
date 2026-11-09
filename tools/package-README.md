# Heimdall v{version} — paquete para {os_label}

Firmwares listos para grabar en la **ESP32-2432S028 (CYD2USB)** y scripts para empezar a trabajar en el proyecto.

| Archivo | Para qué |
|---|---|
| `flash.sh` | Graba un firmware en la placa, **sin instalar ESP-IDF** |
| `firmware/heimdall-assistant-esp32-2432s028.bin` | Asistente de voz Heimdall |
| `firmware/heimdall-face-esp32-2432s028.bin` | Cara del robot (ojos, touch, enlace UART) |
| `tools/heimdall_ask.py` | Hacerle preguntas escritas al asistente por USB |
| `tools/heimdall_face.py` | Manejar la cara desde la computadora (demo, monitor, comandos) |
| `setup-dev.sh` | Instalar el entorno completo de desarrollo para {os_label} |
| `SHA256SUMS` | Checksums de todos los archivos |

## 1. Grabar la placa (5 minutos)

Requisitos: Python 3 y un cable USB **de datos**. Conectá la placa y corré uno de estos:

```sh
./flash.sh assistant      # asistente de voz
./flash.sh face           # cara del robot
```

- El script verifica el checksum, crea su propio entorno Python (`.venv`) y graba la imagen completa.
- Si hay más de un puerto: `./flash.sh face --port <puerto>`. Para ver los comandos sin ejecutarlos: `--dry-run`.
- Se graba la imagen completa: la placa queda **como nueva** (se borran el WiFi guardado y la calibración del touch).
{port_notes}

## 2. Configurar el asistente

1. Conectate con el celular a la red WiFi **`Heimdall-XXXX`** y abrí `http://192.168.4.1`.
2. Elegí tu WiFi de **2,4 GHz** y escribí la contraseña.
3. En las opciones avanzadas, completá **"Custom OTA URL"** con la URL de OTA de tu servidor Heimdall (en la consola: Parámetros → `server.ota`). El firmware publicado no trae ningún servidor propio configurado.
4. La pantalla muestra un código de 6 dígitos: agregalo en la consola del servidor para vincular la placa con un agente.

## 3. Probar sin micrófono

```sh
.venv/bin/python tools/heimdall_ask.py "Contame algo curioso"      # asistente
.venv/bin/python tools/heimdall_face.py demo                       # cara
```

La primera vez que se graba la cara pide calibrar el touch: tocá las 3 cruces.

## 4. Desarrollar

```sh
./setup-dev.sh            # o ./setup-dev.sh --dry-run para ver los pasos
```

Instala las dependencias del sistema, ESP-IDF `release/v6.1`, `clang-format` 23.1.5, clona el repositorio (requiere acceso) y activa el hook de pre-push. Después: `. ~/esp/esp-idf-v6.1/export.sh && cd ~/heimdall && scripts/ci.sh`.

## Verificar el paquete

```sh
{checksum_command}
```
