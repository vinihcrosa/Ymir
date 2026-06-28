# Emscripten-specific build configuration
# Only included when CMAKE_SYSTEM_NAME == "Emscripten"

if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "Emscripten.cmake included but EMSCRIPTEN is not set")
endif()

# Emscripten link flags for the WASM module
set(YMIR_EMSCRIPTEN_LINK_FLAGS
    "-sEXPORT_ES6=1"
    "-sMODULARIZE=1"
    "-sEXPORT_NAME=createYmirModule"
    "-sENVIRONMENT=worker"
    "-sINITIAL_MEMORY=67108864"   # 64 MiB
    "-sALLOW_MEMORY_GROWTH=1"
    "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']"
    "--bind"                       # Embind
    "-sNO_FILESYSTEM=1"
    "-O3"
)

# Disable warnings that don't apply to WASM target
add_compile_options(
    -Wno-unused-command-line-argument
)
