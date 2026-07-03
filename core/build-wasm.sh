#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"

# Target environment for the Emscripten module: "worker" (web, default) or "node".
YMIR_WASM_ENVIRONMENT="${YMIR_WASM_ENVIRONMENT:-worker}"
# Output directory for ymir.js/ymir.wasm. Defaults to the web app's public dir;
# the server build (@ymir/wasm) overrides this to its own package dir.
OUT_DIR="${YMIR_WASM_OUT_DIR:-$REPO_ROOT/apps/web/public/wasm}"
# Separate build dir per environment so web and node builds don't clobber each other.
BUILD_DIR="$SCRIPT_DIR/build-wasm-$YMIR_WASM_ENVIRONMENT"
STAGE_DIR="$SCRIPT_DIR/.wasm-stage-$YMIR_WASM_ENVIRONMENT"

# cmake requires the entry file to be named CMakeLists.txt.
# Stage the WASM-specific CMakeLists alongside the source subdirectories.
rm -rf "$STAGE_DIR"
mkdir -p "$STAGE_DIR"
cp "$SCRIPT_DIR/CMakeLists.wasm.txt" "$STAGE_DIR/CMakeLists.txt"
cp -r "$SCRIPT_DIR/libs"  "$STAGE_DIR/libs"
cp -r "$SCRIPT_DIR/cmake" "$STAGE_DIR/cmake"
cp -r "$SCRIPT_DIR/src"   "$STAGE_DIR/src"

echo "[wasm] Configuring with Emscripten (ENVIRONMENT=$YMIR_WASM_ENVIRONMENT)..."
emcmake cmake \
    -B "$BUILD_DIR" \
    -S "$STAGE_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DYMIR_WASM_ENVIRONMENT="$YMIR_WASM_ENVIRONMENT"

echo "[wasm] Building..."
cmake --build "$BUILD_DIR" --parallel

echo "[wasm] Copying artifacts to $OUT_DIR..."
mkdir -p "$OUT_DIR"
cp "$BUILD_DIR/ymir.js"   "$OUT_DIR/"
cp "$BUILD_DIR/ymir.wasm" "$OUT_DIR/"

rm -rf "$STAGE_DIR"
echo "[wasm] Done. Output: $OUT_DIR"
