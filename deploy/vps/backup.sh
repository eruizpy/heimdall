#!/bin/sh
# Daily backup of the Heimdall server: MySQL dump + secrets/config, kept for KEEP_DAYS.
# Installed on the VPS as /opt/heimdall/backup.sh and run by heimdall-backup.timer.
set -eu

BASE=/opt/heimdall
DEST="$BASE/backups"
KEEP_DAYS="${KEEP_DAYS:-14}"
STAMP=$(date +%Y%m%d-%H%M%S)

cd "$BASE"
. ./.env
umask 077
mkdir -p "$DEST"

TMP="$DEST/.heimdall-$STAMP.tmp"
mkdir -p "$TMP"
trap 'rm -rf "$TMP"' EXIT

docker exec -e MYSQL_PWD="$MYSQL_ROOT_PASSWORD" xiaozhi-esp32-server-db \
    mysqldump -uroot --single-transaction --routines --triggers --default-character-set=utf8mb4 \
    xiaozhi_esp32_server > "$TMP/xiaozhi_esp32_server.sql"
cp .env docker-compose.yml Caddyfile "$TMP/"
cp data/.config.yaml "$TMP/config.yaml"

tar -czf "$DEST/heimdall-$STAMP.tar.gz" -C "$TMP" .
find "$DEST" -name 'heimdall-*.tar.gz' -mtime +"$KEEP_DAYS" -delete

echo "backup ok: $DEST/heimdall-$STAMP.tar.gz ($(du -h "$DEST/heimdall-$STAMP.tar.gz" | cut -f1))"
