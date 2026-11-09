# CLAUDE.md — Heimdall

Heimdall es la versión personalizada de [xiaozhi-esp32](https://github.com/eruizpy/xiaozhi-esp32) (firmware de asistente de voz con IA para ESP32).

Las reglas técnicas del firmware están en `AGENTS.md` y siguen vigentes:

@AGENTS.md

## Regla general: continuidad entre máquinas

El desarrollo alterna entre dos equipos con la misma cuenta de Claude: **Ubuntu (Linux)** y **MacBook (macOS)**. La memoria local de Claude (`~/.claude/...`) **no se sincroniza** entre equipos. La única fuente de verdad compartida es este repositorio en GitHub (`origin/main`).

Por lo tanto, en **cada sesión**:

1. **Al empezar:** ejecutar `git pull --ff-only origin main` antes de cualquier cambio y leer `docs/heimdall/PROGRESS.md` para saber en qué etapa quedó el trabajo.
2. **Durante:** trabajar directamente en `main` (sin ramas). Hacer commit al terminar cada cambio coherente y `git push origin main` enseguida; no hace falta pedir confirmación para estos push.
3. **Memoria del proyecto:** todo lo que deba recordarse entre sesiones (decisiones, configuración elegida, hardware, pendientes, problemas encontrados) va en `docs/heimdall/PROGRESS.md`, **no** en la memoria local de Claude. Actualizarlo y pushearlo antes de cerrar la sesión o cuando el usuario diga que se cambia de equipo.
4. **Al terminar:** verificar que `git status` esté limpio y que `main` coincida con `origin/main`.

## Proceso: SDLC con SDD y TDD

El proceso completo está en `docs/specs/README.md`. Reglas obligatorias:

1. **SDD:** ninguna funcionalidad nueva se implementa sin `docs/specs/NNN-nombre/spec.md` (requisitos y criterios de aceptación), `plan.md` y `tasks.md`, y sin la aprobación del usuario sobre la especificación.
2. **TDD:** para cada tarea, primero el test que falla, después el código mínimo, después el refactor. La lógica se separa del hardware para testearla en la computadora (`tests/host/`, Unity); las herramientas se testean en `scripts/tests/`.
3. **Definición de terminado:** criterios de aceptación cumplidos; tests nuevos y existentes en verde (`ctest --test-dir build-host`, `python3 -m unittest discover -s scripts/tests`); formato con `clang-format` 23.1.5; firmwares compilando; CI (`.github/workflows/heimdall-ci.yml`) en verde; pruebas de hardware registradas en `PROGRESS.md`; documentación y `CHANGELOG.md` al día.
4. **Versionado:** semver con tags `vX.Y.Z` y entrada en `CHANGELOG.md` al cerrar cada etapa.

## Commits y datos privados

- Autor de todos los commits: `eruizpy <eruizpy@gmail.com>`. **No** agregar líneas `Co-Authored-By` ni otra atribución a Claude en commits o PR.
- No commitear el dominio, la IP del VPS ni secretos. Usar marcadores (`<dominio>`, `<ip-vps>`) y variables de entorno locales: `HEIMDALL_OTA_URL` (la lee `scripts/build.py`) y `HEIMDALL_VPS` (scripts de `deploy/vps/`).

## Diferencias entre equipos

- Los comandos y scripts deben funcionar en Linux y macOS (evitar flags exclusivos de GNU como `sed -i` sin sufijo; usar Python o comandos portables).
- La ruta de ESP-IDF y el puerto serie cambian por equipo (`/dev/ttyUSB*` / `/dev/ttyACM*` en Linux, `/dev/cu.usbserial-*` / `/dev/cu.usbmodem*` en macOS). No commitear rutas locales; documentarlas en `PROGRESS.md` si hace falta.
- `sdkconfig`, `build/` y otros archivos generados están en `.gitignore`: se regeneran en cada equipo con `scripts/build.py`.

## Upstream

El remoto `xiaozhi` apunta a `https://github.com/eruizpy/xiaozhi-esp32.git` para traer actualizaciones (`git fetch xiaozhi && git merge xiaozhi/main`). Los remotos no viajan con el clone: agregarlo en cada equipo si se necesita.

`LICENSE` es MIT: conservar los avisos de copyright originales junto al de Esteban RD.
