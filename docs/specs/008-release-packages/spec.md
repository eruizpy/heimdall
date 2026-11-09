# 008 — Paquetes de release para macOS y Linux

| Campo | Valor |
|---|---|
| Estado | Aprobada (pedido explícito de eruizpy, 2026-11-09) |
| Autor | eruizpy |
| Fecha | 2026-11-09 |

## Problema

Un colaborador nuevo tarda horas en poner en marcha Heimdall: instalar ESP-IDF, compilar dos firmwares y descubrir cómo grabarlos. Hace falta un release con paquetes por sistema operativo que permitan **grabar la placa en minutos sin ESP-IDF** y **preparar el entorno de desarrollo con un solo script**.

## Alcance

- Incluye: paquetes `heimdall-vX.Y.Z-macos.tar.gz` y `heimdall-vX.Y.Z-linux.tar.gz` con firmwares precompilados de la CYD (asistente y cara), script de grabación, herramientas de prueba, script de entorno de desarrollo propio de cada sistema, checksums; script que arma los paquetes; release en GitHub.
- No incluye: instaladores gráficos, Windows, firmwares del core S3 (todavía no existen).

## Requisitos funcionales

- **RF1** — Cada paquete contiene: `firmware/heimdall-assistant-esp32-2432s028.bin`, `firmware/heimdall-face-esp32-2432s028.bin`, `flash.sh`, `setup-dev.sh`, `tools/heimdall_ask.py`, `tools/heimdall_face.py`, `README.md` y `SHA256SUMS`.
- **RF2** — `flash.sh assistant|face` graba la imagen combinada en 0x0 usando un entorno Python propio con `esptool` 5.6.0 y `pyserial` 3.5 fijados; detecta el puerto según el sistema; verifica el checksum antes de grabar; `--port` para elegir puerto y `--dry-run` para ver los comandos sin ejecutarlos.
- **RF3** — `setup-dev.sh` instala las dependencias del sistema (Homebrew en macOS, apt en Linux), ESP-IDF `release/v6.1`, `clang-format` 23.1.5 y `pyserial`, clona el repositorio si hace falta y activa el hook de pre-push; `--dry-run` muestra los pasos sin ejecutarlos.
- **RF4** — `scripts/package_release.py --version X.Y.Z` compila los dos firmwares desde el código y arma los dos paquetes con sus checksums en `dist/`.
- **RF5** — El release en GitHub lleva las notas de la versión (del `CHANGELOG.md`) y los paquetes como archivos adjuntos.

## Requisitos no funcionales

- **RNF1 (privacidad)** — Los firmwares del paquete no llevan la URL del servidor propio: el empaquetado se niega a correr con `HEIMDALL_OTA_URL` definida. Cada colaborador carga la URL en el portal WiFi ("Custom OTA URL").
- **RNF2** — Los scripts son POSIX `sh` y funcionan en macOS y Linux.
- **RNF3** — Las versiones de las herramientas están fijadas para que el resultado sea reproducible.

## Criterios de aceptación

- **CA1** — Cada paquete contiene exactamente los archivos de RF1, con los scripts ejecutables. — *Verificación:* `scripts/tests/test_package_release.py`.
- **CA2** — `SHA256SUMS` coincide con los archivos del paquete. — *Verificación:* `scripts/tests/test_package_release.py`.
- **CA3** — El paquete de macOS lleva el setup de macOS y el de Linux el de Linux. — *Verificación:* `scripts/tests/test_package_release.py`.
- **CA4** — Con `HEIMDALL_OTA_URL` definida, el empaquetado falla. — *Verificación:* `scripts/tests/test_package_release.py`.
- **CA5** — `flash.sh --dry-run` muestra el comando de `esptool` con el puerto y el firmware correctos, y rechaza un firmware alterado. — *Verificación:* `scripts/tests/test_package_release.py`.
- **CA6** — `setup-dev.sh --dry-run` de cada sistema muestra la instalación de ESP-IDF `release/v6.1` y `clang-format` 23.1.5. — *Verificación:* `scripts/tests/test_package_release.py`.
- **CA7** — Con el paquete de macOS descomprimido, `flash.sh face` graba la cara y la placa responde `pong`. — *Verificación:* HIL.

## Historial

| Fecha | Cambio |
|---|---|
| 2026-11-09 | Creación |
