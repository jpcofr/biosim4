#!/usr/bin/env bash
# Test video generation and automatically verify output
# Run from project root: ./scripts/test-video.sh

set -e

# Load UI library
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/lib/ui.sh"

cd "$SCRIPT_DIR/.."

ui_box_header "🎬 BioSim4 Video Generation Test" 40
ui_nl

if ! ui_require_file "build/bin/biosim4" "Build it first: cd build && ninja"; then
    exit 1
fi

# Clean old videos
ui_clean "Cleaning old videos..."
rm -f output/images/*.avi output/images/*.mp4

# Run simulation with video-test preset
ui_step "Running simulation (this may take a minute)..."
./build/bin/biosim4 --preset video-test

# Automatically verify videos
ui_nl
ui_inspect "Verifying videos..."
./build/bin/biosim4 --verify-videos

# Ask if user wants to review videos interactively
ui_nl
if ui_confirm "Would you like to review videos interactively?" false; then
    ./build/bin/biosim4 --review-videos
fi

ui_nl
ui_box_footer_success "✅ Video test completed!" 40
