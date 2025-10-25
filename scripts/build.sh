#!/bin/bash
# BioSim4 Build Script
# Flexible build system with multiple options for development workflow

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
BUILD_TYPE="Debug"
BUILD_GENERATOR="Ninja"
BUILD_DIR="./build"
CLEAN_BUILD=false
BUILD_DOCS=false
BUILD_BINARIES=true
ENABLE_VIDEO=true
ENABLE_SANITIZERS=false
ENABLE_THREAD_SANITIZER=false
VERBOSE=false
DRY_RUN=false
SHOW_INFO=false
PARALLEL_JOBS=$(sysctl -n hw.ncpu)
RUN_TESTS=false
TARGET=""

# Script location
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Help message
show_help() {
    cat << EOF
${GREEN}BioSim4 Build Script${NC}
Usage: $0 [OPTIONS]

${BLUE}Build Type:${NC}
  -r, --release              Build in Release mode (default: Debug)
  -d, --debug                Build in Debug mode (explicit)

${BLUE}Build Targets:${NC}
  -b, --binaries-only        Build only binaries (default)
  -D, --docs-only            Build only documentation
  -a, --all                  Build both binaries and documentation
  -t, --target TARGET        Build specific target (e.g., biosim4, test_basic_types)

${BLUE}Build System:${NC}
  -n, --ninja                Use Ninja generator (default)
  -m, --make                 Use Unix Makefiles generator
  -j, --jobs N               Parallel build jobs (default: ${PARALLEL_JOBS})

${BLUE}Build Options:${NC}
  -c, --clean                Clean build (remove CMake cache, keep dependencies)
  --full-clean               Full clean (remove entire build directory)
  --no-video                 Disable video generation support
  --sanitizers               Enable AddressSanitizer + UndefinedBehaviorSanitizer
  --thread-sanitizer         Enable ThreadSanitizer (mutually exclusive with --sanitizers)

${BLUE}Testing:${NC}
  --test                     Run tests after successful build

${BLUE}Information:${NC}
  -i, --info                 Show build configuration info and exit
  -v, --verbose              Verbose build output
  --dry-run                  Show commands without executing
  -h, --help                 Show this help message

${BLUE}Examples:${NC}
  $0                                    # Debug build with Ninja
  $0 -r                                 # Release build
  $0 -c -r                              # Clean Release build
  $0 --docs-only                        # Build only documentation
  $0 -a                                 # Build binaries and docs
  $0 -r --sanitizers                    # Release build with sanitizers
  $0 --target test_basic_types          # Build specific test
  $0 -i                                 # Show current configuration
  $0 -r -j 8                            # Release build with 8 parallel jobs
  $0 --test                             # Build and run tests

${BLUE}Common Workflows:${NC}
  Development cycle:          $0 -d
  Production build:           $0 -r
  Memory leak testing:        $0 -d --sanitizers
  Race condition testing:     $0 -d --thread-sanitizer
  Fresh start:                $0 --full-clean -d
  Documentation update:       $0 --docs-only
  Quick test:                 $0 --target test_basic_types && ./build/bin/test_basic_types

EOF
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -r|--release)
            BUILD_TYPE="Release"
            shift
            ;;
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -b|--binaries-only)
            BUILD_DOCS=false
            BUILD_BINARIES=true
            shift
            ;;
        -D|--docs-only)
            BUILD_DOCS=true
            BUILD_BINARIES=false
            shift
            ;;
        -a|--all)
            BUILD_DOCS=true
            BUILD_BINARIES=true
            shift
            ;;
        -t|--target)
            TARGET="$2"
            shift 2
            ;;
        -n|--ninja)
            BUILD_GENERATOR="Ninja"
            shift
            ;;
        -m|--make)
            BUILD_GENERATOR="Unix Makefiles"
            shift
            ;;
        -j|--jobs)
            PARALLEL_JOBS="$2"
            shift 2
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        --full-clean)
            echo -e "${YELLOW}Removing entire build directory...${NC}"
            rm -rf "$PROJECT_ROOT/$BUILD_DIR"
            echo -e "${GREEN}Full clean complete.${NC}"
            exit 0
            ;;
        --no-video)
            ENABLE_VIDEO=false
            shift
            ;;
        --sanitizers)
            ENABLE_SANITIZERS=true
            ENABLE_THREAD_SANITIZER=false
            shift
            ;;
        --thread-sanitizer)
            ENABLE_THREAD_SANITIZER=true
            ENABLE_SANITIZERS=false
            shift
            ;;
        --test)
            RUN_TESTS=true
            shift
            ;;
        -i|--info)
            SHOW_INFO=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        --dry-run)
            DRY_RUN=true
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            show_help
            exit 1
            ;;
    esac
done

# Navigate to project root
cd "$PROJECT_ROOT"

# Show build configuration info
show_build_info() {
    echo -e "${BLUE}╔══════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║     BioSim4 Build Configuration      ║${NC}"
    echo -e "${BLUE}╚══════════════════════════════════════╝${NC}"
    echo ""
    echo -e "${GREEN}Build Settings:${NC}"
    echo "  Build Type:           $BUILD_TYPE"
    echo "  Generator:            $BUILD_GENERATOR"
    echo "  Build Directory:      $BUILD_DIR"
    echo "  Parallel Jobs:        $PARALLEL_JOBS"
    echo ""
    echo -e "${GREEN}Targets:${NC}"
    echo "  Build Binaries:       $BUILD_BINARIES"
    echo "  Build Documentation:  $BUILD_DOCS"
    if [ -n "$TARGET" ]; then
        echo "  Specific Target:      $TARGET"
    fi
    echo ""
    echo -e "${GREEN}Options:${NC}"
    echo "  Video Generation:     $ENABLE_VIDEO"
    echo "  Sanitizers:           $ENABLE_SANITIZERS"
    echo "  Thread Sanitizer:     $ENABLE_THREAD_SANITIZER"
    echo "  Clean Build:          $CLEAN_BUILD"
    echo "  Run Tests:            $RUN_TESTS"
    echo "  Verbose:              $VERBOSE"
    echo ""
    echo -e "${GREEN}Environment:${NC}"
    echo "  Project Root:         $PROJECT_ROOT"
    echo "  Clang++:              $(command -v clang++ 2>/dev/null || echo 'not found')"
    echo "  CMake:                $(command -v cmake 2>/dev/null || echo 'not found') $(cmake --version 2>/dev/null | head -1 | awk '{print $3}')"
    echo "  Ninja:                $(command -v ninja 2>/dev/null || echo 'not found')"
    echo "  Doxygen:              $(command -v doxygen 2>/dev/null || echo 'not found')"
    echo ""
}

# Validate mutually exclusive options
if [ "$ENABLE_SANITIZERS" = true ] && [ "$ENABLE_THREAD_SANITIZER" = true ]; then
    echo -e "${RED}Error: --sanitizers and --thread-sanitizer are mutually exclusive${NC}"
    exit 1
fi

if [ "$BUILD_DOCS" = true ] && [ "$BUILD_BINARIES" = false ] && [ -n "$TARGET" ]; then
    echo -e "${RED}Error: Cannot specify --target with --docs-only${NC}"
    exit 1
fi

# Show info and exit if requested
if [ "$SHOW_INFO" = true ]; then
    show_build_info
    exit 0
fi

# Print configuration summary
echo -e "${GREEN}Starting BioSim4 Build${NC}"
echo "  Mode: $BUILD_TYPE | Generator: $BUILD_GENERATOR | Jobs: $PARALLEL_JOBS"

# Ensure Homebrew binaries are in PATH
export PATH="/opt/homebrew/bin:$PATH"

# Create build directory if it doesn't exist
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${BLUE}Creating build directory...${NC}"
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Clean build if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo -e "${YELLOW}Performing selective clean (preserving dependencies)...${NC}"
    if [ "$DRY_RUN" = false ]; then
        "$SCRIPT_DIR/clean-build.sh"
    else
        echo "[DRY RUN] Would execute: $SCRIPT_DIR/clean-build.sh"
    fi
fi

# Prepare CMake options
CMAKE_OPTS=(
    "-G" "$BUILD_GENERATOR"
    "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
    "-DENABLE_VIDEO_GENERATION=$([ "$ENABLE_VIDEO" = true ] && echo ON || echo OFF)"
    "-DENABLE_SANITIZERS=$([ "$ENABLE_SANITIZERS" = true ] && echo ON || echo OFF)"
    "-DENABLE_THREAD_SANITIZER=$([ "$ENABLE_THREAD_SANITIZER" = true ] && echo ON || echo OFF)"
    "-DBUILD_DOCUMENTATION=$([ "$BUILD_DOCS" = true ] && echo ON || echo OFF)"
)

# Run CMake configuration
echo -e "${BLUE}Configuring CMake...${NC}"
if [ "$DRY_RUN" = true ]; then
    echo "[DRY RUN] cmake ${CMAKE_OPTS[*]} .."
else
    cmake "${CMAKE_OPTS[@]}" ..
fi

# Build binaries
if [ "$BUILD_BINARIES" = true ]; then
    BUILD_CMD="cmake --build . --config $BUILD_TYPE"

    # Add parallel jobs
    if [ "$BUILD_GENERATOR" = "Ninja" ]; then
        BUILD_CMD="$BUILD_CMD -j $PARALLEL_JOBS"
    else
        BUILD_CMD="$BUILD_CMD -- -j$PARALLEL_JOBS"
    fi

    # Add verbose flag
    if [ "$VERBOSE" = true ]; then
        if [ "$BUILD_GENERATOR" = "Ninja" ]; then
            BUILD_CMD="$BUILD_CMD -v"
        else
            BUILD_CMD="$BUILD_CMD VERBOSE=1"
        fi
    fi

    # Add specific target if specified
    if [ -n "$TARGET" ]; then
        BUILD_CMD="$BUILD_CMD --target $TARGET"
    fi

    echo -e "${BLUE}Building binaries...${NC}"
    if [ "$DRY_RUN" = true ]; then
        echo "[DRY RUN] $BUILD_CMD"
    else
        eval "$BUILD_CMD"
        echo -e "${GREEN}✓ Binaries built successfully${NC}"
    fi
fi

# Build documentation
if [ "$BUILD_DOCS" = true ]; then
    if [ "$BUILD_BINARIES" = false ]; then
        # Need to configure with docs enabled
        echo -e "${BLUE}Configuring for documentation build...${NC}"
        if [ "$DRY_RUN" = true ]; then
            echo "[DRY RUN] cmake -DBUILD_DOCUMENTATION=ON .."
        else
            cmake -DBUILD_DOCUMENTATION=ON ..
        fi
    fi

    echo -e "${BLUE}Building documentation...${NC}"
    if [ "$DRY_RUN" = true ]; then
        echo "[DRY RUN] cmake --build . --target docs"
    else
        cmake --build . --target docs
        echo -e "${GREEN}✓ Documentation built successfully${NC}"
        if [ -d "docs/html" ]; then
            echo "  Documentation available at: $BUILD_DIR/docs/html/index.html"
        fi
    fi
fi

# Run tests if requested
if [ "$RUN_TESTS" = true ] && [ "$BUILD_BINARIES" = true ]; then
    echo -e "${BLUE}Running tests...${NC}"
    if [ "$DRY_RUN" = true ]; then
        echo "[DRY RUN] ctest --output-on-failure"
    else
        ctest --output-on-failure
        echo -e "${GREEN}✓ Tests completed${NC}"
    fi
fi

# Final summary
echo ""
echo -e "${GREEN}╔══════════════════════════════════════╗${NC}"
echo -e "${GREEN}║         Build Complete! 🚀           ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════╝${NC}"
echo ""
if [ "$BUILD_BINARIES" = true ]; then
    echo -e "${BLUE}Binaries:${NC}"
    echo "  Main executable:  $BUILD_DIR/bin/biosim4"
    if [ -z "$TARGET" ]; then
        echo "  Tests:            $BUILD_DIR/bin/test_*"
    fi
    echo ""
    echo -e "${BLUE}Run:${NC}"
    echo "  cd $BUILD_DIR && ./bin/biosim4 ../config/biosim4.toml"
fi

if [ "$BUILD_DOCS" = true ] && [ -d "$BUILD_DIR/docs/html" ]; then
    echo -e "${BLUE}Documentation:${NC}"
    echo "  open $BUILD_DIR/docs/html/index.html"
    echo ""
fi
