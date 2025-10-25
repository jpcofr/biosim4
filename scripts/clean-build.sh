#!/bin/bash
# Selective build cleanup - preserves FetchContent dependency artifacts
# This prevents the need to re-download and rebuild dependencies:
# - googletest, toml11, cli11, raylib, spdlog (includes fmt)
# Without preservation, CMake would re-fetch these on every clean build

set -e  # Exit on error

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

BUILD_DIR="./build"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_ROOT"

if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}Build directory does not exist. Nothing to clean.${NC}"
    exit 0
fi

echo -e "${BLUE}Cleaning build directory (preserving FetchContent dependencies)...${NC}"

cd "$BUILD_DIR"

# Remove CMake cache and configuration files
echo -e "  ${CYAN}•${NC} Removing CMake cache files..."
rm -f CMakeCache.txt
rm -f cmake_install.cmake
rm -f CPackConfig.cmake
rm -f CPackSourceConfig.cmake
rm -f CTestTestfile.cmake
rm -f DartConfiguration.tcl

# Remove build system files
echo -e "  ${CYAN}•${NC} Removing build system files..."
rm -f Makefile
rm -f build.ninja
rm -f rules.ninja
rm -f install_manifest.txt

# Remove all CMakeFiles EXCEPT dependency-related ones
echo -e "  ${CYAN}•${NC} Cleaning CMakeFiles (preserving dependencies)..."
if [ -d "CMakeFiles" ]; then
    find CMakeFiles -mindepth 1 -maxdepth 1 \
      ! -name '*googletest*' \
      ! -name '*toml11*' \
      ! -name '*cli11*' \
      ! -name '*raylib*' \
      ! -name '*spdlog*' \
      -exec rm -rf {} + 2>/dev/null || true
fi

# Remove biosim4 build artifacts (not dependencies)
echo -e "  ${CYAN}•${NC} Removing biosim4 build artifacts..."
rm -rf src/
rm -rf tests/

# Remove bin/ and lib/ directories (will be regenerated)
echo -e "  ${CYAN}•${NC} Removing bin/ and lib/ directories..."
rm -rf bin/
rm -rf lib/

# Remove Testing directory (can be regenerated)
echo -e "  ${CYAN}•${NC} Removing Testing directory..."
rm -rf Testing/

echo ""
echo -e "${GREEN}✓ Build directory cleaned successfully!${NC}"
echo ""
echo -e "${BLUE}Preserved FetchContent dependencies (_deps/):${NC}"

# List preserved dependencies (fmt is bundled with spdlog)
for dep in googletest toml11 cli11 raylib spdlog; do
  if [ -d "_deps/${dep}-src" ]; then
    echo -e "  ${GREEN}✓${NC} ${dep}"
  fi
done

echo ""
echo -e "${YELLOW}To rebuild, run:${NC}"
echo "  cd build"
echo "  cmake -G Ninja .."
echo "  ninja"
