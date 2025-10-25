#!/usr/bin/env bash
# Test video generation and automatically verify output
# Run from project root: ./scripts/test-video.sh

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

cd "$(dirname "$0")/.."

echo -e "${BLUE}╔══════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  🎬 BioSim4 Video Generation Test    ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════╝${NC}"
echo ""

if [ ! -f "build/bin/biosim4" ]; then
    echo -e "${RED}Error: biosim4 binary not found${NC}"
    echo ""
    echo -e "${YELLOW}Build it first:${NC}"
    echo "   cd build && ninja"
    exit 1
fi

# Clean old videos
echo -e "${CYAN}🧹 Cleaning old videos...${NC}"
rm -f output/images/*.avi output/images/*.mp4

# Run simulation with video-test preset
echo -e "${BLUE}▶️  Running simulation (this may take a minute)...${NC}"
./build/bin/biosim4 --preset video-test

# Automatically verify videos
echo ""
echo -e "${CYAN}🔍 Verifying videos...${NC}"
./build/bin/biosim4 --verify-videos

# Ask if user wants to review videos interactively
echo ""
read -p "$(echo -e ${YELLOW})Would you like to review videos interactively? (y/N) $(echo -e ${NC})" -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    ./build/bin/biosim4 --review-videos
fi

echo ""
echo -e "${GREEN}╔══════════════════════════════════════╗${NC}"
echo -e "${GREEN}║   ✅ Video test completed!           ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════╝${NC}"
