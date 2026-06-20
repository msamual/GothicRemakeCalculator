#!/bin/bash
# Run on the Ubuntu server to diagnose why the site is unavailable.

set -euo pipefail

BASE_PATH="/GothicReamakeLockPuzzleCalculator"
PORT="${APP_PORT:-8080}"

COMPOSE_PROJECT_NAME="${COMPOSE_PROJECT_NAME:-gothiclock}"

echo "=== Docker containers (${COMPOSE_PROJECT_NAME}) ==="
docker compose -p "${COMPOSE_PROJECT_NAME}" ps 2>/dev/null \
  || docker-compose -p "${COMPOSE_PROJECT_NAME}" ps 2>/dev/null \
  || echo "docker compose not available"

echo
echo "=== Other containers on port ${PORT} ==="
docker ps --filter "publish=${PORT}" --format 'table {{.Names}}\t{{.Status}}\t{{.Ports}}' 2>/dev/null || true

echo
echo "=== Local app health (must be 200) ==="
curl -fsS "http://127.0.0.1:${PORT}${BASE_PATH}/api/lock/health" && echo || echo "FAIL: app not responding on port ${PORT}"

echo
echo "=== Port ${PORT} listener ==="
ss -tlnp | grep ":${PORT} " || echo "Nothing listening on port ${PORT}"

echo
echo "=== Nginx config for msamual.online ==="
grep -r "GothicReamakeLockPuzzleCalculator\|msamual.online" /etc/nginx/ 2>/dev/null || echo "No nginx config found for this app"

echo
echo "=== Public HTTP check ==="
curl -sS -o /dev/null -w "http://msamual.online${BASE_PATH}/ -> %{http_code}\n" "http://msamual.online${BASE_PATH}/" || true

echo
echo "=== Public HTTPS check ==="
curl -sS -o /dev/null -w "https://msamual.online${BASE_PATH}/ -> %{http_code}\n" "https://msamual.online${BASE_PATH}/" || echo "HTTPS not available (port 443 closed or no certificate)"
