#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TARGET="/etc/nginx/sites-available/msamual.online"
CERT="/etc/letsencrypt/live/msamual.online/fullchain.pem"

if sudo test -f "${CERT}"; then
  SOURCE="${SCRIPT_DIR}/msamual.online.conf"
  echo "Using HTTPS config (certificate found)."
else
  SOURCE="${SCRIPT_DIR}/msamual.online.http-only.conf"
  echo "Using HTTP-only config (no certificate yet)."
  echo "After HTTP works, run: sudo certbot --nginx -d msamual.online"
fi

if [ ! -f "${SOURCE}" ]; then
  echo "Config not found: ${SOURCE}"
  exit 1
fi

awk '
  /^[[:space:]]*#/ { next }
  /^[[:space:]]*$/ && !in_server { next }
  { print }
' "${SOURCE}" | sudo tee "${TARGET}" >/dev/null

sudo ln -sf "${TARGET}" /etc/nginx/sites-enabled/msamual.online
sudo rm -f /etc/nginx/sites-enabled/default

echo "Installed ${TARGET}"
sudo nginx -t
sudo systemctl reload nginx
echo "Nginx reloaded."

echo "Test (local app):"
curl -fsS "http://127.0.0.1:8080/GothicReamakeLockPuzzleCalculator/api/lock/health" && echo

echo "Test (public HTTP):"
curl -fsS "http://msamual.online/GothicReamakeLockPuzzleCalculator/api/lock/health" && echo || echo "HTTP check failed."

echo "Test (public HTTPS):"
if curl -fsS "https://msamual.online/GothicReamakeLockPuzzleCalculator/api/lock/health" && echo; then
  echo "HTTPS OK."
else
  echo "HTTPS failed. If certificates exist, re-run: sudo ./deploy/install-nginx.sh"
  echo "Temporary URL: http://msamual.online/GothicReamakeLockPuzzleCalculator/"
fi
