#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$SCRIPT_DIR/build-wasm"
OUT_DIR="$REPO_ROOT/apps/web/public/wasm"

echo "[wasm] Configuring with Emscripten..."
emcmake cmake \
    -B "$BUILD_DIR" \
    -S "$SCRIPT_DIR" \
    -f CMakeLists.wasm.txt \
    -DCMAKE_BUILD_TYPE=Release

echo "[wasm] Building..."
cmake --build "$BUILD_DIR" --parallel

echo "[wasm] Copying artifacts to $OUT_DIR..."
mkdir -p "$OUT_DIR"
cp "$BUILD_DIR/ymir.js" "$OUT_DIR/"
cp "$BUILD_DIR/ymir.wasm" "$OUT_DIR/"

echo "[wasm] Done. Output: $OUT_DIR"
