#!/bin/sh
# Heimdall: entorno de desarrollo en Linux Debian/Ubuntu (apt, ESP-IDF release/v6.1, herramientas, repo).
#
#   ./setup-dev.sh [--dry-run]
# Variables: IDF_DIR (por defecto ~/esp/esp-idf-v6.1), HEIMDALL_DIR (por defecto ~/heimdall).
set -eu

IDF_BRANCH=release/v6.1
IDF_DIR="${IDF_DIR:-$HOME/esp/esp-idf-v6.1}"
CLANG_FORMAT_VERSION=23.1.5
REPO_URL=https://github.com/eruizpy/heimdall.git
HERE=$(cd "$(dirname "$0")" && pwd)
if [ -f "$HERE/../scripts/ci.sh" ]; then
    HEIMDALL_DIR="${HEIMDALL_DIR:-$(cd "$HERE/.." && pwd)}"  # running from a clone (tools/)
else
    HEIMDALL_DIR="${HEIMDALL_DIR:-$HOME/heimdall}"  # running from a release package
fi

DRY=0
[ "${1:-}" = "--dry-run" ] && DRY=1

run() {
    echo "+ $*"
    if [ "$DRY" -eq 0 ]; then
        "$@"
    fi
}
step() { printf '\n== %s\n' "$1"; }

if [ "$(uname -s)" != "Linux" ] && [ "$DRY" -eq 0 ]; then
    echo "Este script es para Linux; en macOS usá el paquete de macOS." >&2
    exit 1
fi
if [ "$DRY" -eq 0 ] && ! command -v apt-get >/dev/null 2>&1; then
    echo "Este script usa apt (Debian/Ubuntu). En otras distribuciones instalá a mano los paquetes del paso 1." >&2
    exit 1
fi

step "1/5 Dependencias del sistema (apt)"
run sudo apt-get update
run sudo apt-get install -y git wget flex bison gperf python3 python3-pip python3-venv cmake \
    ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0 pipx

step "2/5 ESP-IDF $IDF_BRANCH en $IDF_DIR"
if [ -d "$IDF_DIR/.git" ]; then
    echo "Ya existe; se actualiza."
    run git -C "$IDF_DIR" pull --ff-only
    run git -C "$IDF_DIR" submodule update --init --recursive --depth 1
else
    run mkdir -p "$(dirname "$IDF_DIR")"
    run git clone -b "$IDF_BRANCH" --recursive --depth 1 --shallow-submodules \
        https://github.com/espressif/esp-idf.git "$IDF_DIR"
fi
run "$IDF_DIR/install.sh" esp32

step "3/5 Herramientas del proyecto y acceso al puerto serie"
run pipx install --force "clang-format==$CLANG_FORMAT_VERSION"
run pipx ensurepath
if id -nG | grep -qw dialout; then
    echo "Grupo dialout: ya sos miembro."
else
    echo "Grupo dialout: hace falta para usar /dev/ttyUSB* (después cerrá sesión y volvé a entrar)."
    run sudo usermod -aG dialout "$USER"
fi

step "4/5 Repositorio Heimdall en $HEIMDALL_DIR"
if [ ! -d "$HEIMDALL_DIR/.git" ]; then
    echo "(requiere acceso al repositorio privado)"
    run git clone "$REPO_URL" "$HEIMDALL_DIR"
fi
run git -C "$HEIMDALL_DIR" config core.hooksPath .githooks

step "5/5 Listo"
echo "Abrí una terminal nueva y corré:"
echo "  . $IDF_DIR/export.sh"
echo "  cd $HEIMDALL_DIR && scripts/ci.sh"
echo "La placa aparece como /dev/ttyUSB0 (CH340)."
