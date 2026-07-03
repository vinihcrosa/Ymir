#!/usr/bin/env bash
# Build the Ymir physics engine as a Node-targeted WASM module and stage the
# artifacts (ymir.js / ymir.wasm) inside this package's runtime/ directory.
#
# The C++ sources and CMake live in core/; we drive core/build-wasm.sh with the
# Node environment and this package's output dir. Requires the Emscripten SDK
# (emcc/emcmake) on PATH.
set -euo pipefail

PKG_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "$PKG_DIR/../.." && pwd)"

export YMIR_WASM_ENVIRONMENT="node"
export YMIR_WASM_OUT_DIR="$PKG_DIR/runtime"

bash "$REPO_ROOT/core/build-wasm.sh"
echo "[@ymir/wasm] Node WASM staged at $YMIR_WASM_OUT_DIR"
