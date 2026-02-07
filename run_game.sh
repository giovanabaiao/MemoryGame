#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
BUILD_TYPE="Release"
FETCH_SFML="OFF"
RUN_GAME="ON"

usage() {
  cat <<'EOF'
Build and run the Memory Game.

Usage:
  ./run_game.sh [options] [-- <game-args>]

Options:
  --debug                 Build with Debug config
  --release               Build with Release config (default)
  --fetch-sfml            Enable CMake FetchContent for SFML 3.x
  --build-dir <path>      Custom build directory (default: ./build)
  --no-run                Build only, do not launch the game
  -h, --help              Show this help

Examples:
  ./run_game.sh
  ./run_game.sh --fetch-sfml
  ./run_game.sh --debug --no-run
EOF
}

GAME_ARGS=()
while [[ $# -gt 0 ]]; do
  case "$1" in
    --debug)
      BUILD_TYPE="Debug"
      shift
      ;;
    --release)
      BUILD_TYPE="Release"
      shift
      ;;
    --fetch-sfml)
      FETCH_SFML="ON"
      shift
      ;;
    --build-dir)
      if [[ $# -lt 2 ]]; then
        echo "Error: --build-dir requires a value." >&2
        exit 1
      fi
      BUILD_DIR="$2"
      shift 2
      ;;
    --no-run)
      RUN_GAME="OFF"
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    --)
      shift
      GAME_ARGS=("$@")
      break
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage
      exit 1
      ;;
  esac
done

echo "==> Configuring (${BUILD_TYPE}) in ${BUILD_DIR}"
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DMEMORY_GAME_FETCH_SFML="${FETCH_SFML}"

echo "==> Building"
cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" -j

if [[ "${RUN_GAME}" != "ON" ]]; then
  echo "==> Build complete (run skipped with --no-run)."
  exit 0
fi

GAME_BIN="${BUILD_DIR}/bin/memory_game"
if [[ ! -x "${GAME_BIN}" ]]; then
  GAME_BIN="${BUILD_DIR}/bin/${BUILD_TYPE}/memory_game"
fi

if [[ ! -x "${GAME_BIN}" ]]; then
  echo "Error: built game executable not found in ${BUILD_DIR}/bin" >&2
  exit 1
fi

echo "==> Running ${GAME_BIN}"
if [[ ${#GAME_ARGS[@]} -gt 0 ]]; then
  exec "${GAME_BIN}" "${GAME_ARGS[@]}"
else
  exec "${GAME_BIN}"
fi
