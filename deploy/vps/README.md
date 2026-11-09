# Servidor de Heimdall (VPS)

[xiaozhi-esp32-server](https://github.com/xinnan-tech/xiaozhi-esp32-server) en modo completo (servidor de voz + consola web + MySQL + Redis) detrás de Caddy con HTTPS automático.

| URL | Para qué |
|---|---|
| `https://<dominio>/` | Consola web (智控台) |
| `https://<dominio>/xiaozhi/ota/` | OTA: la placa pregunta acá al arrancar (`CONFIG_OTA_URL`) |
| `wss://<dominio>/xiaozhi/v1/` | Canal de voz de la placa |

Solo Caddy publica puertos (80/443). El firewall (`ufw`) permite 22, 80 y 443.

## VPS

- Linode 2 GB (1 vCPU, 2 GB RAM, 50 GB), Ubuntu 24.04, IP `<ip-vps>`, más 2 GB de swap (`/swapfile`).
- Acceso: `ssh root@<ip-vps>` con la clave `~/.ssh/id_ed25519` de cada equipo (agregarla con `ssh-copy-id`).
- Archivos en `/opt/heimdall/`: `docker-compose.yml`, `Caddyfile`, `.env` (secreto, no está en git), `data/.config.yaml` (lleva el `server.secret`, no está en git), `mysql/`, `caddy/`.

## Instalar desde cero

```sh
# En el VPS: Docker (repo oficial), swap y firewall ya documentados en PROGRESS.md.
mkdir -p /opt/heimdall/data && cd /opt/heimdall
# Copiar desde el repo: docker-compose.yml y Caddyfile (scp deploy/vps/{docker-compose.yml,Caddyfile} root@IP:/opt/heimdall/)
printf "DOMAIN=<dominio>\nTZ=America/Asuncion\nMYSQL_ROOT_PASSWORD=%s\n" "$(openssl rand -hex 24)" > .env && chmod 600 .env
curl -fsSL -o data/.config.yaml https://raw.githubusercontent.com/xinnan-tech/xiaozhi-esp32-server/refs/heads/main/main/xiaozhi-server/config_from_api.yaml
# En data/.config.yaml: manager-api.url = http://xiaozhi-esp32-server-web:8002/xiaozhi
docker compose up -d xiaozhi-esp32-server-db xiaozhi-esp32-server-redis xiaozhi-esp32-server-web
# server.secret: tabla sys_params (param_code = 'server.secret') -> manager-api.secret en data/.config.yaml
docker compose up -d
```

## Consola (una sola vez)

1. Registrar el primer usuario en `https://<dominio>/`: queda como superadministrador.
2. Parámetros: `server.websocket` = `wss://<dominio>/xiaozhi/v1/`, `server.ota` = `https://<dominio>/xiaozhi/ota/`.
3. Modelos: ASR = Groq (Whisper), LLM = Groq (compatible con OpenAI, `https://api.groq.com/openai/v1`), TTS = EdgeTTS con voz en español.
4. Crear el agente Heimdall y vincular la placa con el código de 6 dígitos.
5. Reiniciar el servidor de voz: `docker restart xiaozhi-esp32-server` (libera la RAM de FunASR si quedó cargado).

## Operación

```sh
cd /opt/heimdall
docker compose ps
docker compose logs -f xiaozhi-esp32-server      # voz
docker compose logs -f xiaozhi-esp32-server-web  # consola / OTA
docker compose pull && docker compose up -d      # actualizar
docker stats --no-stream                          # memoria
```

## Respaldos

`backup.sh` (en el VPS: `/opt/heimdall/backup.sh`) guarda cada día a las 03:30 (hora de Asunción) un `.tar.gz` en `/opt/heimdall/backups/` con el dump de MySQL, `.env`, `config.yaml` (con el `server.secret`), `docker-compose.yml` y `Caddyfile`. Conserva 14 días. Lo dispara `heimdall-backup.timer` (systemd).

```sh
# Instalar (desde el repo)
scp deploy/vps/backup.sh root@IP:/opt/heimdall/ && scp deploy/vps/heimdall-backup.{service,timer} root@IP:/etc/systemd/system/
ssh root@IP 'chmod 700 /opt/heimdall/backup.sh && systemctl daemon-reload && systemctl enable --now heimdall-backup.timer'

# Respaldo manual y estado
systemctl start heimdall-backup.service && journalctl -u heimdall-backup.service -n 5
systemctl list-timers heimdall-backup.timer

# Copiar el último respaldo a este equipo (fuera del VPS)
scp "root@IP:$(ssh root@IP 'ls -t /opt/heimdall/backups/*.tar.gz | head -1')" ~/heimdall-backups/
```

Restaurar (pisa la base actual):

```sh
cd /opt/heimdall && . ./.env && mkdir -p /tmp/r && tar -xzf backups/heimdall-FECHA.tar.gz -C /tmp/r
docker exec -i -e MYSQL_PWD="$MYSQL_ROOT_PASSWORD" xiaozhi-esp32-server-db mysql -uroot --default-character-set=utf8mb4 xiaozhi_esp32_server < /tmp/r/xiaozhi_esp32_server.sql
docker exec xiaozhi-esp32-server-redis redis-cli flushall && docker compose restart xiaozhi-esp32-server-web xiaozhi-esp32-server
```

Los respaldos viven en el mismo VPS: protegen contra errores y bases dañadas, no contra perder el VPS. Para eso, copiarlos fuera (comando de arriba) o activar Linode Backups.
