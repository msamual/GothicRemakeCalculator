#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE="${SCRIPT_DIR}/msamual.online.conf"
TARGET="/etc/nginx/sites-available/msamual.online"

if [ ! -f "${SOURCE}" ]; then
  echo "Config not found: ${SOURCE}"
  echo "Run this script from the repository checkout, e.g.:"
  echo "  find /home/github-runner -path '*/deploy/install-nginx.sh' 2>/dev/null"
  exit 1
fi

# nginx ignores comments, but strip them to keep the active config minimal
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

echo "Test (HTTP):"
curl -fsS "http://127.0.0.1:8080/GothicReamakeLockPuzzleCalculator/api/lock/health" && echo
curl -fsS "http://msamual.online/GothicReamakeLockPuzzleCalculator/api/lock/health" && echo

if curl -fsS "https://msamual.online/GothicReamakeLockPuzzleCalculator/api/lock/health" >/dev/null 2>&1; then
  echo "HTTPS is configured."
else
  echo
  echo "HTTPS is NOT configured yet. Browsers often open https:// by default and fail."
  echo "Open in browser: http://msamual.online/GothicReamakeLockPuzzleCalculator/"
  echo "To enable HTTPS:"
  echo "  sudo apt install -y certbot python3-certbot-nginx"
  echo "  sudo certbot --nginx -d msamual.online"
fi
