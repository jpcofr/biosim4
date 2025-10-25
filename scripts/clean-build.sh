#!/bin/bash
# Selective build cleanup - preserves OpenCV and Google Test artifacts
# This prevents the need to rebuild OpenCV from source (30-60 min)
# and avoids re-downloading Google Test

set -e  # Exit on error

BUILD_DIR="./build"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_ROOT"

if [ ! -d "$BUILD_DIR" ]; then
    echo "Build directory does not exist. Nothing to clean."
    exit 0
fi

echo "Cleaning build directory (preserving OpenCV and Google Test artifacts)..."

cd "$BUILD_DIR"

# Remove CMake cache and configuration files
echo "  - Removing CMake cache files..."
rm -f CMakeCache.txt
rm -f cmake_install.cmake
rm -f CPackConfig.cmake
rm -f CPackSourceConfig.cmake
rm -f CTestTestfile.cmake
rm -f DartConfiguration.tcl

# Remove build system files
echo "  - Removing build system files..."
rm -f Makefile
rm -f build.ninja
rm -f rules.ninja
rm -f install_manifest.txt

# Remove all CMakeFiles EXCEPT OpenCV and Google Test related ones
echo "  - Cleaning CMakeFiles (preserving OpenCV and Google Test)..."
if [ -d "CMakeFiles" ]; then
    find CMakeFiles -mindepth 1 -maxdepth 1 ! -name '*opencv*' ! -name '*googletest*' -exec rm -rf {} + 2>/dev/null || true
fi

# Remove biosim4 build artifacts
echo "  - Removing biosim4 build artifacts..."
rm -rf src/

# Remove Testing directory (can be regenerated)
echo "  - Removing Testing directory..."
rm -rf Testing/

echo ""
echo "✓ Build directory cleaned successfully!"
echo ""
echo "Preserved artifacts:"
if [ -d "_deps/opencv-src" ] || [ -d "_deps/opencv-build" ] || [ -d "_deps/opencv-subbuild" ]; then
    echo "OpenCV artifacts:"
    [ -d "_deps/opencv-src" ] && echo "  - _deps/opencv-src/"
    [ -d "_deps/opencv-build" ] && echo "  - _deps/opencv-build/"
    [ -d "_deps/opencv-subbuild" ] && echo "  - _deps/opencv-subbuild/"
fi
if [ -d "_deps/googletest-src" ] || [ -d "_deps/googletest-build" ] || [ -d "_deps/googletest-subbuild" ]; then
    echo "Google Test artifacts:"
    [ -d "_deps/googletest-src" ] && echo "  - _deps/googletest-src/"
    [ -d "_deps/googletest-build" ] && echo "  - _deps/googletest-build/"
    [ -d "_deps/googletest-subbuild" ] && echo "  - _deps/googletest-subbuild/"
fi
echo ""
echo "To rebuild, run:"
echo "  cd build"
echo "  cmake -G Ninja .."
echo "  ninja"
