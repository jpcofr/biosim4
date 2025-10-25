#!/usr/bin/env bash
# Quick test of BioSim4 functionality (no video generation)
# Run from project root: ./scripts/quick-test.sh

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

cd "$(dirname "$0")/.."

echo -e "${BLUE}╔══════════════════════════════════════╗${NC}"
echo -e "${BLUE}║     🚀 BioSim4 Quick Test            ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════╝${NC}"
echo ""

if [ ! -f "build/bin/biosim4" ]; then
    echo -e "${RED}Error: biosim4 binary not found${NC}"
    echo ""
    echo -e "${YELLOW}Build it first:${NC}"
    echo "   cd build && ninja"
    exit 1
fi

# Run with quick preset
echo -e "${BLUE}Running with quick preset...${NC}"
./build/bin/biosim4 --preset quick

echo ""
echo -e "${GREEN}╔══════════════════════════════════════╗${NC}"
echo -e "${GREEN}║   ✅ Quick test completed!           ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════╝${NC}"
