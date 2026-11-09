#!/bin/sh
# Heimdall: entorno de desarrollo en macOS (Homebrew, ESP-IDF release/v6.1, herramientas, repo).
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

if [ "$(uname -s)" != "Darwin" ] && [ "$DRY" -eq 0 ]; then
    echo "Este script es para macOS; en Linux usá el paquete de Linux." >&2
    exit 1
fi

step "1/5 Dependencias del sistema (Homebrew)"
if [ "$DRY" -eq 0 ] && ! command -v brew >/dev/null 2>&1; then
    echo "Falta Homebrew: instalalo desde https://brew.sh y volvé a correr este script." >&2
    exit 1
fi
if [ "$DRY" -eq 0 ] && ! xcode-select -p >/dev/null 2>&1; then
    echo "Faltan las herramientas de línea de comandos: corré 'xcode-select --install' y volvé a empezar." >&2
    exit 1
fi
run brew install git cmake ninja dfu-util ccache python@3.12 pipx

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

step "3/5 Herramientas del proyecto"
run pipx install --force "clang-format==$CLANG_FORMAT_VERSION"
run pipx ensurepath

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
echo "La placa aparece como /dev/cu.usbserial-*; si no aparece, permití el accesorio USB en"
echo "Configuración > Privacidad y seguridad y reconectala."
