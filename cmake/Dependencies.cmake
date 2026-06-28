include(FetchContent)

# ---------------------------------------------------------------------------
# Catch2 v3 — test framework
# ---------------------------------------------------------------------------
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.4
    GIT_SHALLOW    TRUE
)

# ---------------------------------------------------------------------------
# nlohmann/json — JSON parser (header-only)
# ---------------------------------------------------------------------------
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(Catch2 nlohmann_json)

# ---------------------------------------------------------------------------
# CGAL — mesh and geometry library (system package, required from M2)
# ---------------------------------------------------------------------------
find_package(CGAL QUIET)
if(CGAL_FOUND)
    message(STATUS "CGAL found: ${CGAL_VERSION}")
else()
    message(STATUS "CGAL not found — mesh support unavailable (needed from M2)")
endif()
