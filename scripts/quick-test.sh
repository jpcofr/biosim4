#!/usr/bin/env bash
# Quick test of BioSim4 functionality (no video generation)
# Run from project root: ./scripts/quick-test.sh

set -e

cd "$(dirname "$0")/.."

echo "🚀 BioSim4 Quick Test"
echo "===================="
echo ""

if [ ! -f "build/bin/biosim4" ]; then
    echo "❌ biosim4 binary not found. Build it first:"
    echo "   cd build && ninja"
    exit 1
fi

# Run with quick preset
./build/bin/biosim4 --preset quick

echo ""
echo "✅ Quick test completed successfully!"
