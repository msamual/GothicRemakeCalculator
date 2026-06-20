#!/bin/bash

set -euo pipefail

APP_PORT="${APP_PORT:-8080}"
BASE_PATH="${BASE_PATH:-/GothicRemakeLockPuzzleCalculator}"
BASE_PATH="${BASE_PATH%/}"
case "${BASE_PATH}" in
  /*) ;;
  *) BASE_PATH="/${BASE_PATH}" ;;
esac
HEALTH_URL="${HEALTH_URL:-http://127.0.0.1:${APP_PORT}${BASE_PATH}/api/lock/health}"
COMPOSE_PROJECT_NAME="${COMPOSE_PROJECT_NAME:-gothiclock}"

COMPOSE_MODE=""
DOCKER_CMD=()
COMPOSE_CMD=()

run_compose() {
  if [ "${COMPOSE_MODE}" = "plugin" ]; then
    "${DOCKER_CMD[@]}" compose -p "${COMPOSE_PROJECT_NAME}" "$@"
  else
    "${COMPOSE_CMD[@]}" -p "${COMPOSE_PROJECT_NAME}" "$@"
  fi
}

run_compose_legacy_down() {
  local legacy=$1
  if [ "${COMPOSE_MODE}" = "plugin" ]; then
    "${DOCKER_CMD[@]}" compose -p "${legacy}" down --remove-orphans 2>/dev/null || true
  else
    remove_compose_project_containers "${legacy}"
  fi
}

remove_compose_project_containers() {
  local project=$1
  local ids=""

  ids=$("${DOCKER_CMD[@]}" ps -aq --filter "label=com.docker.compose.project=${project}" 2>/dev/null || true)
  if [ -n "${ids}" ]; then
    echo "Removing containers for compose project ${project}..."
    # shellcheck disable=SC2086
    "${DOCKER_CMD[@]}" rm -f ${ids} 2>/dev/null || true
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
    echo "Using: ${DOCKER_CMD[*]} compose -p ${COMPOSE_PROJECT_NAME}"
  elif command -v docker-compose &>/dev/null; then
    COMPOSE_MODE="standalone"
    if [ "${DOCKER_CMD[0]}" = "sudo" ]; then
      COMPOSE_CMD=(sudo docker-compose)
    else
      COMPOSE_CMD=(docker-compose)
    fi
    echo "WARNING: Using deprecated docker-compose v1."
    echo "Install the Compose plugin to avoid ContainerConfig errors: sudo apt install docker-compose-plugin"
    echo "Using: ${COMPOSE_CMD[*]} -p ${COMPOSE_PROJECT_NAME}"
  else
    echo "Neither 'docker compose' nor 'docker-compose' is available."
    echo "Install the Compose plugin: sudo apt install docker-compose-plugin"
    echo "Or standalone Compose: sudo apt install docker-compose"
    exit 1
  fi
}

wait_for_health() {
  echo "Waiting for health check at ${HEALTH_URL}..."
  for attempt in $(seq 1 45); do
    if curl -fsS "${HEALTH_URL}" >/dev/null; then
      echo "Health check passed."
      return 0
    fi

    if [ "${attempt}" -eq 45 ]; then
      echo "Health check failed after 45 attempts."
      echo "API logs:"
      run_compose logs api
      echo "Frontend logs:"
      run_compose logs frontend
      return 1
    fi

    sleep 2
  done
}

echo "Starting Gothic Lock Calculator deployment..."

if ! command -v docker &>/dev/null; then
  echo "Docker is not installed."
  exit 1
fi

setup_docker
export APP_PORT
export BASE_PATH="${BASE_PATH#/}"
export COMPOSE_PROJECT_NAME

echo "Retiring legacy compose projects (one-time cleanup)..."
for legacy in gothicremakecalculator gothiccalculator; do
  run_compose_legacy_down "${legacy}"
done

echo "Building images..."
run_compose build

echo "Starting services (keeping old containers until new ones are ready)..."
if [ "${COMPOSE_MODE}" = "plugin" ]; then
  run_compose up -d --remove-orphans --wait || {
    echo "Compose --wait failed, falling back to manual health check."
    run_compose up -d --remove-orphans
    wait_for_health
  }
else
  # docker-compose v1 cannot recreate containers on modern Docker (KeyError: ContainerConfig).
  echo "Removing old containers before up (docker-compose v1 workaround)..."
  remove_compose_project_containers "${COMPOSE_PROJECT_NAME}"
  run_compose up -d --remove-orphans
  wait_for_health
fi

echo "Running containers:"
run_compose ps

if curl -fsS "${HEALTH_URL}" >/dev/null; then
  echo "Local health check OK."
else
  echo "Local health check FAILED."
  exit 1
fi

if curl -fsS "http://msamual.online/${BASE_PATH}/api/lock/health" >/dev/null 2>&1; then
  echo "Public HTTP health check OK."
else
  echo "WARNING: public URL not reachable. Run: sudo ./deploy/install-nginx.sh"
fi

echo "Deployment completed successfully."
echo "Local URL: http://127.0.0.1:${APP_PORT}/${BASE_PATH}/"
echo "Public URL: https://msamual.online/${BASE_PATH}/"
