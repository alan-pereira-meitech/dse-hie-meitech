#!/usr/bin/env zsh
set -euo pipefail

# Simple helper to run the 1-second streaming example.
# Usage:
#   examples/run_stream.sh <ip-or-url>

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 <ip-or-url>" >&2
  exit 1
fi

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
BUILD_DIR="$ROOT_DIR/build-cmake"
BIN="$BUILD_DIR/dse_stream_1s"

# Configure/build if binary is missing
if [[ ! -x "$BIN" ]]; then
  cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
  cmake --build "$BUILD_DIR" -j 4
fi

exec "$BIN" "$1"
