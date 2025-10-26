#!/bin/bash
# Memory leak testing script for biosim4
# Usage: ./scripts/test-leaks.sh [method]
# Methods: asan (default), leaks, instruments

set -e

# Load UI library
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/lib/ui.sh"

METHOD=${1:-asan}
BUILD_DIR="build"
BINARY="$BUILD_DIR/bin/biosim4"
TEST_CONFIG="tests/configs/leak-test.ini"

ui_box_header "🔍 BioSim4 Memory Leak Testing" 40
ui_nl

case $METHOD in
    asan)
        ui_highlight "Method: AddressSanitizer (ASan)"
        ui_nl

        # Check if binary exists
        if [ ! -f "$BINARY" ]; then
            ui_warn "Binary not found. Building with ASan..."
            rm -rf "$BUILD_DIR"
            mkdir -p "$BUILD_DIR"
            cd "$BUILD_DIR"
            cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON ..
            ninja
            cd ..
        fi

        ui_step "Running simulator with leak detection..."
        ui_keyval "Config" "$TEST_CONFIG"
        ui_nl

        # Run with ASan options and suppressions for macOS system libraries
        export ASAN_OPTIONS="detect_leaks=1:leak_check_at_exit=1:verbosity=0:color=always"
        export LSAN_OPTIONS="suppressions=lsan.supp:report_objects=1"

        if "$BINARY" "$TEST_CONFIG"; then
            ui_nl
            ui_box_footer_success "✅ No memory leaks detected!" 40
        else
            ui_nl
            ui_box_footer_error "❌ Memory leaks or errors found" 40
            ui_warn "See output above for details."
            exit 1
        fi
        ;;

    leaks)
        ui_highlight "Method: macOS leaks command"
        ui_nl

        if [ ! -f "$BINARY" ]; then
            ui_warn "Binary not found. Building with debug symbols..."
            rm -rf "$BUILD_DIR"
            mkdir -p "$BUILD_DIR"
            cd "$BUILD_DIR"
            cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug ..
            ninja
            cd ..
        fi

        ui_step "Running simulator with leaks monitoring..."
        ui_keyval "Config" "$TEST_CONFIG"
        ui_nl

        leaks --atExit -- "$BINARY" "$TEST_CONFIG"
        ;;

    instruments)
        ui_highlight "Method: Xcode Instruments"
        ui_nl

        if [ ! -f "$BINARY" ]; then
            ui_warn "Binary not found. Building with debug symbols..."
            rm -rf "$BUILD_DIR"
            mkdir -p "$BUILD_DIR"
            cd "$BUILD_DIR"
            cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug ..
            ninja
            cd ..
        fi

        ui_step "Launching Instruments..."
        ui_warn "Note: This will open Xcode Instruments GUI"
        ui_nl

        instruments -t Leaks "$BINARY" "$TEST_CONFIG"
        ;;

    *)
        ui_error "Unknown method: $METHOD"
        ui_nl
        ui_help_section "Available methods"
        ui_help_option "asan" "AddressSanitizer (fast, recommended)"
        ui_help_option "leaks" "macOS leaks command"
        ui_help_option "instruments" "Xcode Instruments GUI"
        ui_nl
        ui_info "Usage: $0 [method]"
        exit 1
        ;;
esac
