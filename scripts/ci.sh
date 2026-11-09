#!/bin/sh
# Heimdall local CI: the project does not use GitHub Actions; this script is the CI.
# Runs the same checks on Linux and macOS (docs/specs/README.md, definition of done).
#
#   scripts/ci.sh           tests, format and both firmware builds (load ESP-IDF first)
#   scripts/ci.sh --quick   tests and format only (used by the pre-push hook)
set -eu
cd "$(dirname "$0")/.."

CLANG_FORMAT_VERSION=23.1.5
FORMAT_FILES="face/main/*.c face/main/*.h main/text_query.h tests/host/*.c tests/host/*.cc
main/boards/esp32-2432s028/*.cc main/boards/esp32-2432s028/*.h"
LOGS=build-host/ci-logs

step() { printf '\n== %s\n' "$1"; }
fail() { printf '\nCI FAILED: %s\n' "$1"; exit 1; }

step "Firmware logic tests (host, Unity)"
cmake -S tests/host -B build-host >/dev/null || fail "cmake configure"
cmake --build build-host -j >/dev/null || fail "host test build"
ctest --test-dir build-host --output-on-failure || fail "host tests"
mkdir -p "$LOGS"

step "Tooling tests (Python)"
python3 -m unittest discover -s scripts/tests >"$LOGS/python.log" 2>&1 || { tail -30 "$LOGS/python.log"; fail "Python tests"; }
grep -E "^(Ran|OK|FAILED)" "$LOGS/python.log"

step "Format (clang-format $CLANG_FORMAT_VERSION)"
version=$(clang-format --version | sed -E 's/.*version ([0-9.]+).*/\1/')
[ "$version" = "$CLANG_FORMAT_VERSION" ] || echo "warning: clang-format $version installed, $CLANG_FORMAT_VERSION expected (pip install clang-format==$CLANG_FORMAT_VERSION)"
# shellcheck disable=SC2086
clang-format --dry-run -Werror $FORMAT_FILES || fail "format (run clang-format -i on the files above)"
echo "ok"

if [ "${1:-}" != "--quick" ]; then
    command -v idf.py >/dev/null 2>&1 || fail "ESP-IDF not loaded: . ~/esp/esp-idf-v6.1/export.sh"

    step "Assistant firmware (esp32-2432s028)"
    python3 scripts/build.py esp32-2432s028 --name esp32-2432s028 --language es-ES >"$LOGS/assistant.log" 2>&1 \
        || { tail -30 "$LOGS/assistant.log"; fail "assistant firmware build"; }
    grep -a "binary size" "$LOGS/assistant.log" | grep -v Bootloader | tail -1

    step "Face firmware (face/)"
    [ -f face/sdkconfig ] || (cd face && idf.py set-target esp32 >/dev/null 2>&1)
    (cd face && idf.py build) >"$LOGS/face.log" 2>&1 || { tail -30 "$LOGS/face.log"; fail "face firmware build"; }
    grep -a "binary size" "$LOGS/face.log" | grep -v Bootloader | tail -1
fi

step "CI OK"
