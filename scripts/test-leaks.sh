#!/bin/bash
# Memory leak testing script for biosim4
# Usage: ./scripts/test-leaks.sh [method]
# Methods: asan (default), leaks, instruments

set -e

METHOD=${1:-asan}
BUILD_DIR="build"
BINARY="$BUILD_DIR/src/biosim4"
TEST_CONFIG="tests/configs/leak-test.ini"

echo "🔍 BioSim4 Memory Leak Testing"
echo "================================"
echo ""

case $METHOD in
    asan)
        echo "Method: AddressSanitizer (ASan)"
        echo ""
        
        # Check if binary exists
        if [ ! -f "$BINARY" ]; then
            echo "❌ Binary not found. Building with ASan..."
            rm -rf "$BUILD_DIR"
            mkdir -p "$BUILD_DIR"
            cd "$BUILD_DIR"
            cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON ..
            ninja
            cd ..
        fi
        
        echo "Running simulator with leak detection..."
        echo "Config: $TEST_CONFIG"
        echo ""
        
        # Run with ASan options and suppressions for macOS system libraries
        export ASAN_OPTIONS="detect_leaks=1:leak_check_at_exit=1:verbosity=0:color=always"
        export LSAN_OPTIONS="suppressions=lsan.supp:report_objects=1"
        
        if "$BINARY" "$TEST_CONFIG"; then
            echo ""
            echo "✅ No memory leaks detected!"
        else
            echo ""
            echo "❌ Memory leaks or errors found. See output above."
            exit 1
        fi
        ;;
        
    leaks)
        echo "Method: macOS leaks command"
        echo ""
        
        if [ ! -f "$BINARY" ]; then
            echo "❌ Binary not found. Building with debug symbols..."
            rm -rf "$BUILD_DIR"
            mkdir -p "$BUILD_DIR"
            cd "$BUILD_DIR"
            cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug ..
            ninja
            cd ..
        fi
        
        echo "Running simulator with leaks monitoring..."
        echo "Config: $TEST_CONFIG"
        echo ""
        
        leaks --atExit -- "$BINARY" "$TEST_CONFIG"
        ;;
        
    instruments)
        echo "Method: Xcode Instruments"
        echo ""
        
        if [ ! -f "$BINARY" ]; then
            echo "❌ Binary not found. Building with debug symbols..."
            rm -rf "$BUILD_DIR"
            mkdir -p "$BUILD_DIR"
            cd "$BUILD_DIR"
            cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug ..
            ninja
            cd ..
        fi
        
        echo "Launching Instruments..."
        echo "Note: This will open Xcode Instruments GUI"
        echo ""
        
        instruments -t Leaks "$BINARY" "$TEST_CONFIG"
        ;;
        
    valgrind)
        echo "Method: Valgrind (via Docker)"
        echo ""
        echo "⚠️  This is slow but comprehensive"
        echo ""
        
        # Check if Dockerfile has valgrind
        if ! grep -q "valgrind" Dockerfile; then
            echo "❌ Dockerfile doesn't include valgrind."
            echo "Add this to your Dockerfile:"
            echo ""
            echo "RUN apt-get update && apt-get install -yqq \\"
            echo "    valgrind \\"
            echo "    && apt-get clean"
            echo ""
            exit 1
        fi
        
        echo "Building Docker image with valgrind..."
        docker build -t biosim4-valgrind .
        
        echo ""
        echo "Running valgrind in Docker..."
        docker run --rm -v "$(pwd)":/app biosim4-valgrind bash -c \
            "mkdir -p build && cd build && \
             cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug .. && \
             ninja && \
             valgrind --leak-check=full --show-leak-kinds=all \
             --track-origins=yes --verbose \
             --log-file=../valgrind-out.txt \
             ./src/biosim4 ../$TEST_CONFIG"
        
        echo ""
        echo "Valgrind output saved to: valgrind-out.txt"
        
        if grep -q "definitely lost: 0 bytes" valgrind-out.txt && \
           grep -q "indirectly lost: 0 bytes" valgrind-out.txt; then
            echo "✅ No memory leaks detected!"
        else
            echo "❌ Memory leaks found. Check valgrind-out.txt"
            exit 1
        fi
        ;;
        
    *)
        echo "❌ Unknown method: $METHOD"
        echo ""
        echo "Available methods:"
        echo "  asan        - AddressSanitizer (fast, recommended)"
        echo "  leaks       - macOS leaks command"
        echo "  instruments - Xcode Instruments GUI"
        echo "  valgrind    - Valgrind via Docker (slow but thorough)"
        echo ""
        echo "Usage: $0 [method]"
        exit 1
        ;;
esac
