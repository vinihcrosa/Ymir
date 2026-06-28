#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$SCRIPT_DIR/build-wasm"
STAGE_DIR="$SCRIPT_DIR/.wasm-stage"
OUT_DIR="$REPO_ROOT/apps/web/public/wasm"

# cmake requires the entry file to be named CMakeLists.txt.
# Stage the WASM-specific CMakeLists alongside the source subdirectories.
rm -rf "$STAGE_DIR"
mkdir -p "$STAGE_DIR"
cp "$SCRIPT_DIR/CMakeLists.wasm.txt" "$STAGE_DIR/CMakeLists.txt"
cp -r "$SCRIPT_DIR/libs"  "$STAGE_DIR/libs"
cp -r "$SCRIPT_DIR/cmake" "$STAGE_DIR/cmake"
cp -r "$SCRIPT_DIR/src"   "$STAGE_DIR/src"

echo "[wasm] Configuring with Emscripten..."
emcmake cmake \
    -B "$BUILD_DIR" \
    -S "$STAGE_DIR" \
    -DCMAKE_BUILD_TYPE=Release

echo "[wasm] Building..."
cmake --build "$BUILD_DIR" --parallel

echo "[wasm] Copying artifacts to $OUT_DIR..."
mkdir -p "$OUT_DIR"
cp "$BUILD_DIR/ymir.js"   "$OUT_DIR/"
cp "$BUILD_DIR/ymir.wasm" "$OUT_DIR/"

rm -rf "$STAGE_DIR"
echo "[wasm] Done. Output: $OUT_DIR"
