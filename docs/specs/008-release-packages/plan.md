# 008 — Plan técnico

## Diseño

| Pieza | Responsabilidad |
|---|---|
| `tools/flash.sh` | Grabación sin ESP-IDF: venv con esptool/pyserial fijados, detección de puerto, verificación de checksum |
| `tools/setup-macos.sh`, `tools/setup-linux.sh` | Entorno de desarrollo completo por sistema |
| `tools/package-README.md` | Guía rápida que va dentro del paquete |
| `scripts/package_release.py` | Compilar firmwares (opcional `--skip-build`) y armar los paquetes; funciones puras testeables |
| `gh release create` | Publicar el release con notas y adjuntos |

Paquete: `heimdall-vX.Y.Z-<os>/` con `firmware/`, `tools/`, `flash.sh`, `setup-dev.sh`, `README.md`, `SHA256SUMS`, comprimido en `.tar.gz` (conserva los permisos de ejecución).

## Decisiones

| Decisión | Alternativas descartadas | Motivo |
|---|---|---|
| Imagen combinada en 0x0 | Varias imágenes con offsets | Un solo comando, menos errores |
| venv propio con esptool fijado | Exigir ESP-IDF | Grabar no debería requerir instalar 4 GB |
| Firmware sin URL propia + portal WiFi | Binario con el dominio | El dominio no se publica (spec 003) |
| `.tar.gz` | `.zip` | Conserva permisos de ejecución en macOS y Linux |

## Estrategia de pruebas

| Nivel | Qué se prueba |
|---|---|
| Unitario (Python) | Armado, contenido, checksums, privacidad, `--dry-run` de los scripts |
| HIL | Grabar la cara con el paquete de macOS y recibir `pong` |
