#!/usr/bin/env bash
#
# run-all-tests.sh — run the full Ymir test suite.
#
# Suites:
#   cpp   C++ physics engine — CMake build + ctest        (core/)
#   js    Monorepo unit tests — vitest via turbo          (pnpm test)
#   e2e   Web end-to-end — Playwright (opt-in, needs deps) (@ymir/web)
#
# Usage:
#   scripts/run-all-tests.sh            # cpp + js
#   scripts/run-all-tests.sh --e2e      # cpp + js + e2e
#   scripts/run-all-tests.sh --cpp      # cpp only
#   scripts/run-all-tests.sh --js       # js only
#   scripts/run-all-tests.sh --e2e-only # e2e only
#
# Exit code is non-zero if any selected suite fails. Suites run to completion
# (no fail-fast) so you see every failure in one pass.

set -uo pipefail

# --- locate repo root -------------------------------------------------------
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

# --- colors (disabled when not a TTY) --------------------------------------
if [[ -t 1 ]]; then
    BOLD=$'\033[1m'; RED=$'\033[31m'; GREEN=$'\033[32m'; CYAN=$'\033[36m'; RESET=$'\033[0m'
else
    BOLD=''; RED=''; GREEN=''; CYAN=''; RESET=''
fi

section() { printf '\n%s========== %s ==========%s\n' "$BOLD$CYAN" "$1" "$RESET"; }

# --- suite selection --------------------------------------------------------
RUN_CPP=1
RUN_JS=1
RUN_E2E=0

case "${1:-}" in
    --cpp)      RUN_CPP=1; RUN_JS=0; RUN_E2E=0 ;;
    --js)       RUN_CPP=0; RUN_JS=1; RUN_E2E=0 ;;
    --e2e-only) RUN_CPP=0; RUN_JS=0; RUN_E2E=1 ;;
    --e2e)      RUN_CPP=1; RUN_JS=1; RUN_E2E=1 ;;
    "")         ;;
    -h|--help)  sed -n '2,23p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *)          echo "${RED}Unknown option: $1${RESET}"; exit 2 ;;
esac

declare -a RESULTS
FAILED=0

record() { # name, status
    if [[ "$2" -eq 0 ]]; then
        RESULTS+=("${GREEN}PASS${RESET}  $1")
    else
        RESULTS+=("${RED}FAIL${RESET}  $1")
        FAILED=1
    fi
}

# --- C++ (CMake + ctest) ----------------------------------------------------
run_cpp() {
    section "C++ engine (core/) — CMake + ctest"
    if ! command -v cmake >/dev/null 2>&1; then
        echo "${RED}cmake not found on PATH${RESET}"; return 1
    fi
    local gen=()
    command -v ninja >/dev/null 2>&1 && gen=(-G Ninja)
    cmake -S core -B core/build "${gen[@]}" -DYMIR_BUILD_TESTS=ON -DYMIR_BUILD_APPS=OFF || return 1
    cmake --build core/build || return 1
    ctest --test-dir core/build --output-on-failure || return 1
}

# --- JS (turbo → vitest) ----------------------------------------------------
run_js() {
    section "Monorepo unit tests — pnpm test (vitest)"
    if ! command -v pnpm >/dev/null 2>&1; then
        echo "${RED}pnpm not found on PATH${RESET}"; return 1
    fi
    [[ -d node_modules ]] || pnpm install || return 1
    pnpm test || return 1
}

# --- E2E (Playwright) -------------------------------------------------------
run_e2e() {
    section "Web E2E — Playwright (@ymir/web)"
    if ! command -v pnpm >/dev/null 2>&1; then
        echo "${RED}pnpm not found on PATH${RESET}"; return 1
    fi
    [[ -d node_modules ]] || pnpm install || return 1
    pnpm --filter @ymir/web test:e2e || return 1
}

# --- run selected suites ----------------------------------------------------
[[ "$RUN_CPP" -eq 1 ]] && { run_cpp; record "C++ (ctest)" $?; }
[[ "$RUN_JS"  -eq 1 ]] && { run_js;  record "JS  (vitest)" $?; }
[[ "$RUN_E2E" -eq 1 ]] && { run_e2e; record "E2E (playwright)" $?; }

# --- summary ----------------------------------------------------------------
section "Summary"
for line in "${RESULTS[@]}"; do printf '  %s\n' "$line"; done

if [[ "$FAILED" -eq 0 ]]; then
    printf '\n%sAll selected suites passed.%s\n' "$GREEN$BOLD" "$RESET"
else
    printf '\n%sOne or more suites failed.%s\n' "$RED$BOLD" "$RESET"
fi
exit "$FAILED"
