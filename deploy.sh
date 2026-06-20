#!/bin/bash

set -euo pipefail

APP_PORT="${APP_PORT:-8080}"
HEALTH_URL="${HEALTH_URL:-http://127.0.0.1:${APP_PORT}/api/lock/health}"
COMPOSE="docker compose"

echo "Starting Gothic Lock Calculator deployment..."

if ! command -v docker &>/dev/null; then
  echo "Docker is not installed."
  exit 1
fi

if docker compose version &>/dev/null 2>&1; then
  COMPOSE="docker compose"
elif command -v docker-compose &>/dev/null; then
  COMPOSE="docker-compose"
else
  echo "Neither 'docker compose' nor 'docker-compose' is available."
  echo "Install the Compose plugin: sudo apt install docker-compose-plugin"
  echo "Or standalone Compose: sudo apt install docker-compose"
  exit 1
fi

echo "Using: ${COMPOSE}"

export APP_PORT

echo "Stopping existing containers..."
$COMPOSE down || true

echo "Building and starting services..."
$COMPOSE up --build -d

echo "Waiting for health check at ${HEALTH_URL}..."
for attempt in $(seq 1 30); do
  if curl -fsS "${HEALTH_URL}" >/dev/null; then
    echo "Health check passed."
    break
  fi

  if [ "${attempt}" -eq 30 ]; then
    echo "Health check failed after 30 attempts."
    echo "API logs:"
    $COMPOSE logs api
    echo "Frontend logs:"
    $COMPOSE logs frontend
    exit 1
  fi

  sleep 2
done

echo "Deployment completed successfully."
echo "Application is available at: http://127.0.0.1:${APP_PORT}"
echo "Running containers:"
$COMPOSE ps
