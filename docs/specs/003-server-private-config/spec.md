# 003 — Servidor propio y configuración privada

| Campo | Valor |
|---|---|
| Estado | Verificada (retroactiva) |
| Autor | eruizpy |
| Fecha | 2026-11-09 |

## Problema

Heimdall necesita su propio servidor de voz y consola, y el repositorio no debe exponer el dominio, la IP ni secretos de ese servidor.

## Alcance

- Incluye: despliegue en contenedores detrás de un proxy HTTPS, respaldos diarios y copia fuera del servidor, URL del servidor inyectada al compilar.
- No incluye: la migración a AWS (especificación 007).

## Requisitos funcionales

- **RF1** — El firmware toma la URL de OTA de la variable `HEIMDALL_OTA_URL` al compilar; sin la variable, conserva el valor por defecto.
- **RF2** — Ningún archivo versionado contiene el dominio, la IP ni secretos del servidor.
- **RF3** — Respaldo diario de la base y la configuración, 14 días de retención, restauración documentada y probada.
- **RF4** — `pull-backups.sh` copia los respaldos a la computadora sin borrar copias locales.

## Requisitos no funcionales

- **RNF1** — Solo el proxy publica puertos (80/443); certificados RSA para acelerar el TLS de la ESP32.
- **RNF2** — Funciona con cualquier proveedor x86_64 de 2 GB + swap.

## Criterios de aceptación

- **CA1** — Con `HEIMDALL_OTA_URL` definida, la configuración generada incluye `CONFIG_OTA_URL` con ese valor; sin ella, no la incluye. — *Verificación:* `scripts/tests/test_heimdall.py`.
- **CA2** — La configuración de la placa en el repo no fija `CONFIG_OTA_URL`. — *Verificación:* `scripts/tests/test_heimdall.py`.
- **CA3** — El respaldo se restaura en una base temporal con las mismas filas y acentos. — *Verificación:* manual (2026-11-09).
- **CA4** — La placa conecta por WebSocket en ≤ 3 s. — *Verificación:* HIL, 1,8 s (2026-11-09).

## Historial

| Fecha | Cambio |
|---|---|
| 2026-11-09 | Especificación retroactiva |
