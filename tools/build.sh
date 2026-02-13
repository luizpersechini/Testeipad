#!/bin/bash
# C&C: Tiberian Dawn Modern Port - Build Script
#
# Usage:
#   ./tools/build.sh              # Desktop build (Release)
#   ./tools/build.sh debug        # Desktop build (Debug)
#   ./tools/build.sh web          # Web/Emscripten build
#   ./tools/build.sh clean        # Clean build artifacts

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"

case "${1:-release}" in
    debug)
        echo "=== Building Desktop (Debug) ==="
        mkdir -p "${BUILD_DIR}/debug"
        cd "${BUILD_DIR}/debug"
        cmake "${PROJECT_ROOT}" -DCMAKE_BUILD_TYPE=Debug
        cmake --build . -- -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
        echo "=== Build complete: ${BUILD_DIR}/debug/cnc_td ==="
        ;;

    release)
        echo "=== Building Desktop (Release) ==="
        mkdir -p "${BUILD_DIR}/release"
        cd "${BUILD_DIR}/release"
        cmake "${PROJECT_ROOT}" -DCMAKE_BUILD_TYPE=Release
        cmake --build . -- -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
        echo "=== Build complete: ${BUILD_DIR}/release/cnc_td ==="
        ;;

    web)
        echo "=== Building Web (Emscripten) ==="
        if ! command -v emcmake &> /dev/null; then
            echo "Error: Emscripten SDK not found. Install from https://emscripten.org"
            exit 1
        fi
        mkdir -p "${BUILD_DIR}/web"
        cd "${BUILD_DIR}/web"
        emcmake cmake "${PROJECT_ROOT}" -DBUILD_WEB=ON
        emmake make -j$(nproc 2>/dev/null || echo 4)
        echo "=== Web build complete: ${BUILD_DIR}/web/cnc_td.html ==="
        echo "    Run: python3 -m http.server 8080 --directory ${BUILD_DIR}/web"
        ;;

    clean)
        echo "=== Cleaning build artifacts ==="
        rm -rf "${BUILD_DIR}"
        echo "=== Clean complete ==="
        ;;

    *)
        echo "Usage: $0 [debug|release|web|clean]"
        exit 1
        ;;
esac
