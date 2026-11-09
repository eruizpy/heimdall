# Servidor de Heimdall

[xiaozhi-esp32-server](https://github.com/xinnan-tech/xiaozhi-esp32-server) en modo completo (servidor de voz + consola web + MySQL + Redis) detrás de Caddy con HTTPS automático.

| URL | Para qué |
|---|---|
| `https://<dominio>/` | Consola web (智控台) |
| `https://<dominio>/xiaozhi/ota/` | OTA: la placa pregunta acá al arrancar (`CONFIG_OTA_URL`) |
| `wss://<dominio>/xiaozhi/v1/` | Canal de voz de la placa |

Solo Caddy publica puertos (80/443). Abrir 22, 80 y 443 en el firewall del proveedor y en `ufw`.

## Servidor (cualquier proveedor)

Funciona en cualquier máquina virtual Linux con Docker: AWS, Azure, Google Cloud, Linode, DigitalOcean, Hetzner o una PC propia.

| Requisito | Mínimo | Nota |
|---|---|---|
| CPU | 1 vCPU **x86_64** | Las imágenes del servidor se publican solo para x86_64: evitar instancias ARM (AWS Graviton `t4g`, Azure `Dps`, GCP `t2a`) salvo que compiles las imágenes |
| RAM | 2 GB + 2 GB de swap | El modo completo usa ~800 MB con ASR en la nube; los modelos locales necesitan bastante más |
| Disco | 25 GB | La imagen del servidor de voz ocupa ~11 GB |
| Sistema | Ubuntu 24.04 LTS | Otras distribuciones con Docker también sirven |
| Red | IP pública fija y puertos 80/443 abiertos | El 22 solo para administrar |

Equivalencias orientativas por proveedor:

| Proveedor | Instancia de 2 GB | IP fija | Firewall del proveedor | Snapshots / respaldos |
|---|---|---|---|---|
| AWS | EC2 `t3.small` | Elastic IP | Security Group | EBS Snapshots / AWS Backup |
| Azure | VM `B1ms` | IP pública estática | Network Security Group | Azure Backup |
| Google Cloud | Compute Engine `e2-small` | IP externa estática | Reglas de firewall de VPC | Snapshots de disco |
| Linode (Akamai) | Linode 2 GB | Incluida | Cloud Firewall | Linode Backups |

En el DNS del dominio, crear un registro **A** para `<dominio>` (y `www`) apuntando a `<ip-vps>`. Caddy obtiene el certificado HTTPS solo cuando el registro ya resuelve.

Archivos en `/opt/heimdall/`: `docker-compose.yml`, `Caddyfile`, `.env` (secreto, no está en git), `data/.config.yaml` (lleva el `server.secret`, no está en git), `mysql/`, `caddy/`, `backups/`.

Acceso: `ssh root@<ip-vps>` (o el usuario que cree el proveedor, por ejemplo `ubuntu` en AWS y GCP) con la clave `~/.ssh/id_ed25519` de cada equipo. En los equipos de desarrollo conviene `export HEIMDALL_VPS=root@<ip-vps>` para los scripts de esta carpeta.

## Instalar desde cero

Preparar el servidor (una vez):

```sh
# Swap de 2 GB
fallocate -l 2G /swapfile && chmod 600 /swapfile && mkswap /swapfile && swapon /swapfile
echo "/swapfile none swap sw 0 0" >> /etc/fstab

# Docker (repositorio oficial)
apt-get update && apt-get install -y ca-certificates curl
install -m 0755 -d /etc/apt/keyrings
curl -fsSL https://download.docker.com/linux/ubuntu/gpg -o /etc/apt/keyrings/docker.asc
echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/ubuntu $(. /etc/os-release && echo $VERSION_CODENAME) stable" > /etc/apt/sources.list.d/docker.list
apt-get update && apt-get install -y docker-ce docker-ce-cli containerd.io docker-compose-plugin

# Firewall del sistema (además del firewall del proveedor)
ufw allow 22/tcp && ufw allow 80/tcp && ufw allow 443/tcp && ufw allow 443/udp && ufw --force enable

# Zona horaria (la usan los respaldos)
timedatectl set-timezone America/Asuncion
```

Desplegar Heimdall:

```sh
mkdir -p /opt/heimdall/data && cd /opt/heimdall
# Copiar desde el repo: scp deploy/vps/{docker-compose.yml,Caddyfile} $HEIMDALL_VPS:/opt/heimdall/
printf "DOMAIN=<dominio>\nTZ=America/Asuncion\nMYSQL_ROOT_PASSWORD=%s\n" "$(openssl rand -hex 24)" > .env && chmod 600 .env
curl -fsSL -o data/.config.yaml https://raw.githubusercontent.com/xinnan-tech/xiaozhi-esp32-server/refs/heads/main/main/xiaozhi-server/config_from_api.yaml
# En data/.config.yaml: manager-api.url = http://xiaozhi-esp32-server-web:8002/xiaozhi
docker compose up -d xiaozhi-esp32-server-db xiaozhi-esp32-server-redis xiaozhi-esp32-server-web caddy
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
scp deploy/vps/backup.sh $HEIMDALL_VPS:/opt/heimdall/ && scp deploy/vps/heimdall-backup.{service,timer} $HEIMDALL_VPS:/etc/systemd/system/
ssh $HEIMDALL_VPS 'chmod 700 /opt/heimdall/backup.sh && systemctl daemon-reload && systemctl enable --now heimdall-backup.timer'

# Respaldo manual y estado
systemctl start heimdall-backup.service && journalctl -u heimdall-backup.service -n 5
systemctl list-timers heimdall-backup.timer

```

Copia fuera del VPS: desde la MacBook o el Ubuntu, `deploy/vps/pull-backups.sh` trae los respaldos nuevos a `~/heimdall-backups/` (fuera del repo: tienen secretos; nunca borra copias locales). Correrlo de vez en cuando, por ejemplo una vez por semana.

Restaurar (pisa la base actual):

```sh
cd /opt/heimdall && . ./.env && mkdir -p /tmp/r && tar -xzf backups/heimdall-FECHA.tar.gz -C /tmp/r
docker exec -i -e MYSQL_PWD="$MYSQL_ROOT_PASSWORD" xiaozhi-esp32-server-db mysql -uroot --default-character-set=utf8mb4 xiaozhi_esp32_server < /tmp/r/xiaozhi_esp32_server.sql
docker exec xiaozhi-esp32-server-redis redis-cli flushall && docker compose restart xiaozhi-esp32-server-web xiaozhi-esp32-server
```

Los respaldos viven en el mismo VPS: protegen contra errores y bases dañadas, no contra perder el VPS. Para eso está `pull-backups.sh`, o los snapshots/respaldos del proveedor (ver la tabla de arriba).
