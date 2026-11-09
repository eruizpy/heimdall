#!/bin/sh
# Copy the Heimdall server backups from the VPS to this machine (macOS or Linux).
# Backups contain secrets (.env, server.secret): they go to ~/heimdall-backups, never into the repo.
#
# Usage: deploy/vps/pull-backups.sh [user@host]   (or set HEIMDALL_VPS=user@host)
set -eu

REMOTE="${1:-${HEIMDALL_VPS:?pass user@host or set HEIMDALL_VPS}}"
DEST="${HEIMDALL_BACKUP_DIR:-$HOME/heimdall-backups}"

mkdir -p "$DEST"
chmod 700 "$DEST"

# Only copy archives not already here; local copies are never deleted by this script.
rsync -a --ignore-existing --include='heimdall-*.tar.gz' --exclude='*' "$REMOTE:/opt/heimdall/backups/" "$DEST/"
chmod 600 "$DEST"/heimdall-*.tar.gz

LATEST=$(ls -t "$DEST"/heimdall-*.tar.gz | head -1)
tar -tzf "$LATEST" | grep -q 'xiaozhi_esp32_server.sql' || { echo "error: $LATEST has no SQL dump" >&2; exit 1; }
echo "ok: $(ls "$DEST"/heimdall-*.tar.gz | wc -l | tr -d ' ') backups in $DEST, latest $(basename "$LATEST")"
