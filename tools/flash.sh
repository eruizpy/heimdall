#!/bin/sh
# Heimdall: graba un firmware precompilado en la ESP32-2432S028 (CYD) sin instalar ESP-IDF.
# Usa un entorno Python propio (.venv) con esptool y pyserial en versiones fijas.
#
#   ./flash.sh assistant|face [--port PUERTO] [--dry-run]
set -eu

ESPTOOL_VERSION=5.6.0
PYSERIAL_VERSION=3.5
BAUD=460800
HERE=$(cd "$(dirname "$0")" && pwd)

usage() {
    echo "Uso: ./flash.sh assistant|face [--port PUERTO] [--dry-run]" >&2
    echo "  assistant  asistente de voz Heimdall" >&2
    echo "  face       cara del robot" >&2
    exit 2
}

[ $# -ge 1 ] || usage
case "$1" in
    assistant) FW=firmware/heimdall-assistant-esp32-2432s028.bin ;;
    face) FW=firmware/heimdall-face-esp32-2432s028.bin ;;
    *) usage ;;
esac
shift

PORT=""
DRY=0
while [ $# -gt 0 ]; do
    case "$1" in
        --port)
            [ $# -ge 2 ] || usage
            PORT=$2
            shift 2
            ;;
        --dry-run)
            DRY=1
            shift
            ;;
        *) usage ;;
    esac
done

run() {
    echo "+ $*"
    if [ "$DRY" -eq 0 ]; then
        "$@"
    fi
}

cd "$HERE"

# 1. El firmware tiene que coincidir con SHA256SUMS.
if command -v sha256sum >/dev/null 2>&1; then
    actual=$(sha256sum "$FW" | cut -d' ' -f1)
else
    actual=$(shasum -a 256 "$FW" | cut -d' ' -f1)
fi
expected=$(grep "  $FW\$" SHA256SUMS | cut -d' ' -f1 || true)
if [ -z "$expected" ] || [ "$expected" != "$actual" ]; then
    echo "Error: el checksum de $FW no coincide con SHA256SUMS (archivo dañado o modificado)." >&2
    exit 1
fi
echo "Checksum OK: $FW"

# 2. Puerto serie.
if [ -z "$PORT" ]; then
    case "$(uname -s)" in
        Darwin) candidates=$(ls /dev/cu.usbserial-* /dev/cu.wchusbserial* /dev/cu.usbmodem* 2>/dev/null || true) ;;
        *) candidates=$(ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null || true) ;;
    esac
    count=$(printf '%s\n' "$candidates" | grep -c . || true)
    if [ "$count" -eq 0 ]; then
        echo "Error: no encontré la placa conectada." >&2
        case "$(uname -s)" in
            Darwin) echo "En macOS: permití el accesorio USB (Configuración > Privacidad y seguridad) y reconectá la placa." >&2 ;;
            *) echo "En Linux: revisá el cable USB de datos; el puerto suele ser /dev/ttyUSB0." >&2 ;;
        esac
        exit 1
    elif [ "$count" -gt 1 ]; then
        echo "Hay varios puertos serie; elegí uno con --port:" >&2
        printf '  %s\n' $candidates >&2
        exit 1
    fi
    PORT=$candidates
fi
if [ "$DRY" -eq 0 ] && [ "$(uname -s)" = "Linux" ] && [ ! -w "$PORT" ]; then
    echo "Aviso: sin permiso de escritura en $PORT. Agregate al grupo dialout:" >&2
    echo "  sudo usermod -aG dialout \$USER   (y volvé a iniciar sesión)" >&2
fi
echo "Puerto: $PORT"

# 3. Entorno Python propio con versiones fijas.
if [ ! -x .venv/bin/python ]; then
    run python3 -m venv .venv
fi
run .venv/bin/python -m pip install --quiet --disable-pip-version-check \
    "esptool==$ESPTOOL_VERSION" "pyserial==$PYSERIAL_VERSION"

# 4. Grabar la imagen combinada desde 0x0.
run .venv/bin/esptool --chip esp32 --port "$PORT" --baud "$BAUD" \
    --before default-reset --after hard-reset write-flash 0x0 "$FW"

echo
echo "Listo. Próximos pasos:"
if [ "$FW" = "firmware/heimdall-assistant-esp32-2432s028.bin" ]; then
    echo "  1. Conectate a la red WiFi Heimdall-XXXX y abrí http://192.168.4.1"
    echo "  2. Elegí tu WiFi de 2,4 GHz y, en opciones avanzadas, cargá la URL de tu servidor en \"Custom OTA URL\""
    echo "  3. Probalo sin micrófono: .venv/bin/python tools/heimdall_ask.py \"Hola\" --port $PORT"
else
    echo "  1. Si es la primera vez, tocá las 3 cruces para calibrar el touch"
    echo "  2. Probala: .venv/bin/python tools/heimdall_face.py --port $PORT demo"
fi
