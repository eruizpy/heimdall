# Changelog

Formato basado en [Keep a Changelog](https://keepachangelog.com/es-ES/1.1.0/); versionado [semántico](https://semver.org/lang/es/).

## [Sin publicar]

### Cambiado
- El CI pasa a ser local: `scripts/ci.sh` (tests, formato y firmwares) y el hook `.githooks/pre-push`. El proyecto no usa GitHub Actions.

### Corregido
- `scripts/ci.sh` borra las variables de entorno de git que exporta un hook: dentro del pre-push, los tests que crean repos temporales actuaban sobre el repo real (marcaron `core.bare=true` y cambiaron `user.name`).

### Eliminado
- Workflow `heimdall-ci.yml` de GitHub Actions; `build.yml` vuelve a su versión original.

## [0.1.0] — 2026-11-09

Primera versión de Heimdall: asistente de voz en la ESP32-2432S028 con servidor propio, cara del robot y proceso de desarrollo SDD/TDD.

### Agregado
- Placa `esp32-2432s028` (CYD2USB): pantalla ST7789, touch como botón, audio por DAC + INMP441, red WiFi `Heimdall-XXXX`.
- Consultas de texto por puerto serie (`Application::SendTextQuery`, `scripts/heimdall_ask.py`) — spec 001.
- Firmware de la cara del robot (`face/`): ojos animados, 10 expresiones, touch calibrado y protocolo UART — spec 002.
- Servidor propio en contenedores con HTTPS, respaldos diarios y copia fuera del servidor; URL del servidor por `HEIMDALL_OTA_URL` — spec 003.
- Proceso SDLC con SDD y TDD (`docs/specs/`), tests de host con Unity (`tests/host/`), tests de herramientas y CI en GitHub Actions.
- README de Heimdall con arquitectura de hardware y software, plan de migración a AWS IoT (`us-east-1`) y lista de compras por etapas.

### Cambiado
- Conexión del dispositivo al servidor de ~10 s a ~1,8 s: salida del ahorro de energía WiFi antes de conectar y certificados RSA en el proxy.
- El workflow con la matriz completa de placas pasa a ejecución manual; cada push corre el CI de Heimdall.

### Corregido
- La cara se congelaba al recibir una emoción vacía (`{"t":"emo","v":""}`): bucle infinito en la búsqueda de palabras. Encontrado por un test nuevo.
- La lectura serie esperaba a llenar el búfer antes de procesar una línea.

[Sin publicar]: https://github.com/eruizpy/heimdall/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/eruizpy/heimdall/releases/tag/v0.1.0
