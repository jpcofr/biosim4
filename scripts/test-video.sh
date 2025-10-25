#!/usr/bin/env bash
# Test video generation and automatically verify output
# Run from project root: ./scripts/test-video.sh

set -e

cd "$(dirname "$0")/.."

echo "🎬 BioSim4 Video Generation Test"
echo "================================"
echo ""

if [ ! -f "build/bin/biosim4" ]; then
    echo "❌ biosim4 binary not found. Build it first:"
    echo "   cd build && ninja"
    exit 1
fi

# Clean old videos
echo "🧹 Cleaning old videos..."
rm -f output/images/*.avi output/images/*.mp4

# Run simulation with video-test preset
echo "▶️  Running simulation (this may take a minute)..."
./build/bin/biosim4 --preset video-test

# Automatically verify videos
echo ""
echo "🔍 Verifying videos..."
./build/bin/biosim4 --verify-videos

# Ask if user wants to review videos interactively
echo ""
read -p "Would you like to review videos interactively? (y/N) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    ./build/bin/biosim4 --review-videos
fi

echo ""
echo "✅ Video test completed!"
