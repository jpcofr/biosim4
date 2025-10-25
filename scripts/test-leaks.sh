#!/bin/bash
# Memory leak testing script for biosim4
# Usage: ./scripts/test-leaks.sh [method]
# Methods: asan (default), leaks, instruments

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

METHOD=${1:-asan}
BUILD_DIR="build"
BINARY="$BUILD_DIR/bin/biosim4"
TEST_CONFIG="tests/configs/leak-test.ini"

echo -e "${BLUE}╔══════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  🔍 BioSim4 Memory Leak Testing      ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════╝${NC}"
echo ""

case $METHOD in
    asan)
        echo -e "${CYAN}Method: AddressSanitizer (ASan)${NC}"
        echo ""

        # Check if binary exists
        if [ ! -f "$BINARY" ]; then
            echo -e "${YELLOW}Binary not found. Building with ASan...${NC}"
            rm -rf "$BUILD_DIR"
            mkdir -p "$BUILD_DIR"
            cd "$BUILD_DIR"
            cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON ..
            ninja
            cd ..
        fi

        echo -e "${BLUE}Running simulator with leak detection...${NC}"
        echo -e "Config: ${CYAN}$TEST_CONFIG${NC}"
        echo ""

        # Run with ASan options and suppressions for macOS system libraries
        export ASAN_OPTIONS="detect_leaks=1:leak_check_at_exit=1:verbosity=0:color=always"
        export LSAN_OPTIONS="suppressions=lsan.supp:report_objects=1"

        if "$BINARY" "$TEST_CONFIG"; then
            echo ""
            echo -e "${GREEN}╔══════════════════════════════════════╗${NC}"
            echo -e "${GREEN}║   ✅ No memory leaks detected!       ║${NC}"
            echo -e "${GREEN}╚══════════════════════════════════════╝${NC}"
        else
            echo ""
            echo -e "${RED}╔══════════════════════════════════════╗${NC}"
            echo -e "${RED}║   ❌ Memory leaks or errors found    ║${NC}"
            echo -e "${RED}╚══════════════════════════════════════╝${NC}"
            echo -e "${YELLOW}See output above for details.${NC}"
            exit 1
        fi
        ;;

    leaks)
        echo -e "${CYAN}Method: macOS leaks command${NC}"
        echo ""

        if [ ! -f "$BINARY" ]; then
            echo -e "${YELLOW}Binary not found. Building with debug symbols...${NC}"
            rm -rf "$BUILD_DIR"
            mkdir -p "$BUILD_DIR"
            cd "$BUILD_DIR"
            cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug ..
            ninja
            cd ..
        fi

        echo -e "${BLUE}Running simulator with leaks monitoring...${NC}"
        echo -e "Config: ${CYAN}$TEST_CONFIG${NC}"
        echo ""

        leaks --atExit -- "$BINARY" "$TEST_CONFIG"
        ;;

    instruments)
        echo -e "${CYAN}Method: Xcode Instruments${NC}"
        echo ""

        if [ ! -f "$BINARY" ]; then
            echo -e "${YELLOW}Binary not found. Building with debug symbols...${NC}"
            rm -rf "$BUILD_DIR"
            mkdir -p "$BUILD_DIR"
            cd "$BUILD_DIR"
            cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug ..
            ninja
            cd ..
        fi

        echo -e "${BLUE}Launching Instruments...${NC}"
        echo -e "${YELLOW}Note: This will open Xcode Instruments GUI${NC}"
        echo ""

        instruments -t Leaks "$BINARY" "$TEST_CONFIG"
        ;;

    *)
        echo -e "${RED}Unknown method: $METHOD${NC}"
        echo ""
        echo -e "${YELLOW}Available methods:${NC}"
        echo "  asan        - AddressSanitizer (fast, recommended)"
        echo "  leaks       - macOS leaks command"
        echo "  instruments - Xcode Instruments GUI"
        echo ""
        echo -e "${BLUE}Usage:${NC} $0 [method]"
        exit 1
        ;;
esac
