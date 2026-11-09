# 003 — Plan técnico

## Diseño

| Pieza | Responsabilidad |
|---|---|
| `deploy/vps/docker-compose.yml`, `Caddyfile` | Servidor de voz, consola, MySQL, Redis y proxy HTTPS |
| `deploy/vps/backup.sh` + timer de systemd | Respaldo diario |
| `deploy/vps/pull-backups.sh` | Copia de respaldos fuera del servidor (`HEIMDALL_VPS`) |
| `scripts/build.py` | Lee `HEIMDALL_OTA_URL` y agrega `CONFIG_OTA_URL` |

## Decisiones

| Decisión | Alternativas descartadas | Motivo |
|---|---|---|
| Variable de entorno para la URL | Archivo de configuración versionado | El dominio no debe quedar en git |
| Certificados RSA-2048 | ECDSA (por defecto) | La ESP32 acelera RSA; ECDSA P-384 costaba ~3 s por conexión |

## Estrategia de pruebas

| Nivel | Qué se prueba |
|---|---|
| Unitario (Python) | Inyección de la URL y ausencia de URL fija en el repo |
| Manual | Restauración de respaldos |
| HIL | Tiempo de conexión |
