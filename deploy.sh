#!/bin/bash

set -euo pipefail

APP_PORT="${APP_PORT:-8080}"
BASE_PATH="${BASE_PATH:-/GothicReamakeLockPuzzleCalculator}"
BASE_PATH="${BASE_PATH%/}"
case "${BASE_PATH}" in
  /*) ;;
  *) BASE_PATH="/${BASE_PATH}" ;;
esac
HEALTH_URL="${HEALTH_URL:-http://127.0.0.1:${APP_PORT}${BASE_PATH}/api/lock/health}"

COMPOSE_MODE=""
DOCKER_CMD=()
COMPOSE_CMD=()

run_compose() {
  if [ "${COMPOSE_MODE}" = "plugin" ]; then
    "${DOCKER_CMD[@]}" compose "$@"
  else
    "${COMPOSE_CMD[@]}" "$@"
  fi
}

setup_docker() {
  if docker info &>/dev/null 2>&1; then
    DOCKER_CMD=(docker)
  elif sudo -n docker info &>/dev/null 2>&1; then
    echo "Using sudo for Docker access."
    DOCKER_CMD=(sudo docker)
  else
    echo "Cannot access Docker daemon (permission denied)."
    echo "Add the runner user to the docker group, then restart the runner:"
    echo "  sudo usermod -aG docker $(whoami)"
    echo "  sudo ./svc.sh stop && sudo ./svc.sh start   # in actions-runner dir"
    exit 1
  fi

  if "${DOCKER_CMD[@]}" compose version &>/dev/null 2>&1; then
    COMPOSE_MODE="plugin"
    echo "Using: ${DOCKER_CMD[*]} compose"
  elif command -v docker-compose &>/dev/null; then
    COMPOSE_MODE="standalone"
    if [ "${DOCKER_CMD[0]}" = "sudo" ]; then
      COMPOSE_CMD=(sudo docker-compose)
    else
      COMPOSE_CMD=(docker-compose)
    fi
    echo "Using: ${COMPOSE_CMD[*]}"
  else
    echo "Neither 'docker compose' nor 'docker-compose' is available."
    echo "Install the Compose plugin: sudo apt install docker-compose-plugin"
    echo "Or standalone Compose: sudo apt install docker-compose"
    exit 1
  fi
}

echo "Starting Gothic Lock Calculator deployment..."

if ! command -v docker &>/dev/null; then
  echo "Docker is not installed."
  exit 1
fi

setup_docker
export APP_PORT
export BASE_PATH="${BASE_PATH#/}"

echo "Stopping existing containers..."
run_compose down || true

echo "Building and starting services..."
run_compose up --build -d

echo "Waiting for health check at ${HEALTH_URL}..."
for attempt in $(seq 1 30); do
  if curl -fsS "${HEALTH_URL}" >/dev/null; then
    echo "Health check passed."
    break
  fi

  if [ "${attempt}" -eq 30 ]; then
    echo "Health check failed after 30 attempts."
    echo "API logs:"
    run_compose logs api
    echo "Frontend logs:"
    run_compose logs frontend
    exit 1
  fi

  sleep 2
done

echo "Deployment completed successfully."
echo "Local URL: http://127.0.0.1:${APP_PORT}/${BASE_PATH}/"
echo "Public URL: https://msamual.online/${BASE_PATH}/"
echo "Running containers:"
run_compose ps
