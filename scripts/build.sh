#!/bin/bash
# BioSim4 Build Script
# Flexible build system with multiple options for development workflow

set -e  # Exit on error

# Load UI library
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/lib/ui.sh"

PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

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
SHOW_DEPS=false
PARALLEL_JOBS=$(sysctl -n hw.ncpu)
RUN_TESTS=false
RUN_QUICK_TEST=false
TARGET=""

# Help message
show_help() {
    ui_success "BioSim4 Build Script"
    echo "Usage: $0 [OPTIONS]"
    ui_nl
    ui_help_section "Build Type"
    ui_help_option "-r, --release" "Build in Release mode (default: Debug)"
    ui_help_option "-d, --debug" "Build in Debug mode (default)"
    ui_nl
    ui_help_section "Build Targets"
    ui_help_option "-b, --binaries-only" "Build only binaries (default)"
    ui_help_option "-D, --docs-only" "Build only documentation"
    ui_help_option "-a, --all" "Build both binaries and documentation"
    ui_help_option "-t, --target TARGET" "Build specific target (e.g., biosim4, test_basic_types)"
    ui_nl
    ui_help_section "Build System"
    ui_help_option "-n, --ninja" "Use Ninja generator (default)"
    ui_help_option "-m, --make" "Use Unix Makefiles generator"
    ui_help_option "-j, --jobs N" "Parallel build jobs (default: ${PARALLEL_JOBS})"
    ui_nl
    ui_help_section "Build Options"
    ui_help_option "-c, --clean" "Clean build (preserves FetchContent dependencies)"
    ui_help_option "--full-clean" "Full clean (remove entire build directory)"
    ui_help_option "--no-video" "Disable video generation (default: enabled)"
    ui_help_option "--sanitizers" "Enable AddressSanitizer + UndefinedBehaviorSanitizer (default: off)"
    ui_help_option "--thread-sanitizer" "Enable ThreadSanitizer (mutually exclusive with --sanitizers, default: off)"
    ui_nl
    ui_help_section "Testing"
    ui_help_option "--test" "Run tests after successful build (default: off)"
    ui_help_option "--quick-test" "Run biosim4 with quick preset after build (default: off)"
    ui_nl
    ui_help_section "Information"
    ui_help_option "-i, --info" "Show build configuration info and exit"
    ui_help_option "--show-deps" "Show preserved FetchContent dependencies and exit"
    ui_help_option "-v, --verbose" "Verbose build output"
    ui_help_option "--dry-run" "Show commands without executing"
    ui_help_option "-h, --help" "Show this help message"
    ui_nl
    ui_help_section "Examples"
    ui_help_example "$0" "Debug build with Ninja"
    ui_help_example "$0 -r" "Release build"
    ui_help_example "$0 -c -r" "Clean Release build (preserves deps)"
    ui_help_example "$0 --docs-only" "Build only documentation"
    ui_help_example "$0 -a" "Build binaries and docs"
    ui_help_example "$0 -r --sanitizers" "Release build with sanitizers"
    ui_help_example "$0 --target test_basic_types" "Build specific test"
    ui_help_example "$0 -i" "Show current configuration"
    ui_help_example "$0 --show-deps" "Show preserved dependencies"
    ui_help_example "$0 -r -j 8" "Release build with 8 parallel jobs"
    ui_help_example "$0 --test" "Build and run tests"
    ui_help_example "$0 --quick-test" "Build and run quick simulation"
    ui_nl
    ui_help_section "Common Workflows"
    ui_keyval "Development cycle" "$0 -d"
    ui_keyval "Production build" "$0 -r"
    ui_keyval "Memory leak testing" "$0 -d --sanitizers"
    ui_keyval "Race condition testing" "$0 -d --thread-sanitizer"
    ui_keyval "Fresh start" "$0 --full-clean -d"
    ui_keyval "Documentation update" "$0 --docs-only"
    ui_keyval "Quick test" "$0 --quick-test"
    ui_keyval "Specific unit test" "$0 --target test_basic_types && ./build/bin/test_basic_types"
    ui_nl
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
            ui_warn "Removing entire build directory..."
            rm -rf "$PROJECT_ROOT/$BUILD_DIR"
            ui_success "Full clean complete."
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
        --quick-test)
            RUN_QUICK_TEST=true
            shift
            ;;
        -i|--info)
            SHOW_INFO=true
            shift
            ;;
        --show-deps)
            SHOW_DEPS=true
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
            ui_error "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Navigate to project root
cd "$PROJECT_ROOT"

# Function to perform selective clean (preserves FetchContent dependencies)
perform_selective_clean() {
    if [ ! -d "$BUILD_DIR" ]; then
        ui_warn "Build directory does not exist. Nothing to clean."
        return
    fi

    ui_step "Cleaning build directory (preserving FetchContent dependencies)..."

    local current_dir=$(pwd)
    cd "$BUILD_DIR"

    # Remove CMake cache and configuration files
    ui_bullet "Removing CMake cache files..."
    rm -f CMakeCache.txt cmake_install.cmake CPackConfig.cmake CPackSourceConfig.cmake CTestTestfile.cmake DartConfiguration.tcl

    # Remove build system files
    ui_bullet "Removing build system files..."
    rm -f Makefile build.ninja rules.ninja install_manifest.txt

    # Remove all CMakeFiles EXCEPT dependency-related ones
    ui_bullet "Cleaning CMakeFiles (preserving dependencies)..."
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
    ui_bullet "Removing biosim4 build artifacts..."
    rm -rf src/ tests/

    # Remove bin/ and lib/ directories (will be regenerated)
    ui_bullet "Removing bin/ and lib/ directories..."
    rm -rf bin/ lib/

    # Remove Testing directory (can be regenerated)
    ui_bullet "Removing Testing directory..."
    rm -rf Testing/

    ui_nl
    ui_success "✓ Selective clean completed!"
    ui_nl
    ui_info "Preserved FetchContent dependencies (_deps/):"

    # List preserved dependencies
    for dep in googletest toml11 cli11 raylib spdlog; do
        if [ -d "_deps/${dep}-src" ]; then
            ui_check "${dep}"
        fi
    done
    ui_nl

    cd "$current_dir"
}

# Show build configuration info
show_build_info() {
    ui_box_header "BioSim4 Build Configuration" 40
    ui_nl
    ui_subsection "Build Settings"
    ui_keyval "Build Type" "$BUILD_TYPE"
    ui_keyval "Generator" "$BUILD_GENERATOR"
    ui_keyval "Build Directory" "$BUILD_DIR"
    ui_keyval "Parallel Jobs" "$PARALLEL_JOBS"
    ui_nl
    ui_subsection "Targets"
    ui_keyval "Build Binaries" "$BUILD_BINARIES"
    ui_keyval "Build Documentation" "$BUILD_DOCS"
    if [ -n "$TARGET" ]; then
        ui_keyval "Specific Target" "$TARGET"
    fi
    ui_nl
    ui_subsection "Options"
    ui_keyval "Video Generation" "$ENABLE_VIDEO"
    ui_keyval "Sanitizers" "$ENABLE_SANITIZERS"
    ui_keyval "Thread Sanitizer" "$ENABLE_THREAD_SANITIZER"
    ui_keyval "Clean Build" "$CLEAN_BUILD"
    ui_keyval "Run Tests" "$RUN_TESTS"
    ui_keyval "Run Quick Test" "$RUN_QUICK_TEST"
    ui_keyval "Verbose" "$VERBOSE"
    ui_nl
    ui_subsection "Environment"
    ui_keyval "Project Root" "$PROJECT_ROOT"
    ui_keyval "Clang++" "$(command -v clang++ 2>/dev/null || echo 'not found')"
    ui_keyval "CMake" "$(command -v cmake 2>/dev/null || echo 'not found') $(cmake --version 2>/dev/null | head -1 | awk '{print $3}')"
    ui_keyval "Ninja" "$(command -v ninja 2>/dev/null || echo 'not found')"
    ui_keyval "Doxygen" "$(command -v doxygen 2>/dev/null || echo 'not found')"
    ui_nl
}

# Show preserved FetchContent dependencies
show_dependencies() {
    ui_box_header "Preserved FetchContent Dependencies" 40
    ui_nl

    if [ ! -d "$BUILD_DIR/_deps" ]; then
        ui_warn "No build directory found at: $BUILD_DIR"
        ui_warn "Run a build first to download dependencies."
        ui_nl
        return
    fi

    cd "$BUILD_DIR"

    local found_any=false
    local deps=("googletest" "toml11" "cli11" "raylib" "spdlog")

    ui_subsection "Dependencies Status"
    for dep in "${deps[@]}"; do
        if [ -d "_deps/${dep}-src" ]; then
            found_any=true
            local dep_path="_deps/${dep}-src"
            local size=$(du -sh "$dep_path" 2>/dev/null | cut -f1)

            # Try to get version info if available
            local version=""
            if [ -f "$dep_path/CMakeLists.txt" ]; then
                version=$(grep -i "project.*VERSION" "$dep_path/CMakeLists.txt" | head -1 | sed -E 's/.*VERSION[[:space:]]+([0-9.]+).*/\1/' 2>/dev/null || echo "")
            fi

            if [ -n "$version" ]; then
                ui_check "${dep} (v${version}) - ${size}"
            else
                ui_check "${dep} - ${size}"
            fi
        else
            ui_cross "${dep} (not downloaded)"
        fi
    done

    ui_nl

    if [ "$found_any" = true ]; then
        local total_size=$(du -sh "_deps" 2>/dev/null | cut -f1)
        ui_info "Total dependencies size: ${total_size}"
        ui_nl
        ui_success "Note: These dependencies are preserved during clean builds (-c flag)"
        echo "      Use --full-clean to remove all dependencies"
    else
        ui_warn "No dependencies downloaded yet."
        ui_warn "Run a build to download and compile dependencies."
    fi

    ui_nl
}


# Validate mutually exclusive options
if [ "$ENABLE_SANITIZERS" = true ] && [ "$ENABLE_THREAD_SANITIZER" = true ]; then
    ui_die "Error: --sanitizers and --thread-sanitizer are mutually exclusive" 1
fi

if [ "$BUILD_DOCS" = true ] && [ "$BUILD_BINARIES" = false ] && [ -n "$TARGET" ]; then
    ui_die "Error: Cannot specify --target with --docs-only" 1
fi

# Show info and exit if requested
if [ "$SHOW_INFO" = true ]; then
    show_build_info
    exit 0
fi

# Show dependencies and exit if requested
if [ "$SHOW_DEPS" = true ]; then
    show_dependencies
    exit 0
fi

# Print configuration summary
ui_success "Starting BioSim4 Build"
echo "  Mode: $BUILD_TYPE | Generator: $BUILD_GENERATOR | Jobs: $PARALLEL_JOBS"

# Ensure Homebrew binaries are in PATH
export PATH="/opt/homebrew/bin:$PATH"

# Create build directory if it doesn't exist
if [ ! -d "$BUILD_DIR" ]; then
    ui_step "Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Clean build if requested (selective clean preserving FetchContent dependencies)
if [ "$CLEAN_BUILD" = true ]; then
    if [ "$DRY_RUN" = true ]; then
        echo "[DRY RUN] Would perform selective clean (preserving FetchContent dependencies)"
    else
        perform_selective_clean
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
ui_step "Configuring CMake..."
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

    ui_step "Building binaries..."
    if [ "$DRY_RUN" = true ]; then
        echo "[DRY RUN] $BUILD_CMD"
    else
        eval "$BUILD_CMD"
        ui_check "Binaries built successfully"
    fi
fi

# Build documentation
if [ "$BUILD_DOCS" = true ]; then
    if [ "$BUILD_BINARIES" = false ]; then
        # Need to configure with docs enabled
        ui_step "Configuring for documentation build..."
        if [ "$DRY_RUN" = true ]; then
            echo "[DRY RUN] cmake -DBUILD_DOCUMENTATION=ON .."
        else
            cmake -DBUILD_DOCUMENTATION=ON ..
        fi
    fi

    ui_step "Building documentation..."
    if [ "$DRY_RUN" = true ]; then
        echo "[DRY RUN] cmake --build . --target docs"
    else
        cmake --build . --target docs
        ui_check "Documentation built successfully"
        if [ -d "docs/html" ]; then
            echo "  Documentation available at: $BUILD_DIR/docs/html/index.html"
        fi
    fi
fi

# Run tests if requested
if [ "$RUN_TESTS" = true ] && [ "$BUILD_BINARIES" = true ]; then
    ui_step "Running tests..."
    if [ "$DRY_RUN" = true ]; then
        echo "[DRY RUN] ctest --output-on-failure"
    else
        ctest --output-on-failure
        ui_check "Tests completed"
    fi
fi

# Run quick test if requested
if [ "$RUN_QUICK_TEST" = true ] && [ "$BUILD_BINARIES" = true ]; then
    ui_step "Running biosim4 with quick preset..."
    if [ "$DRY_RUN" = true ]; then
        echo "[DRY RUN] ./bin/biosim4 --preset quick"
    else
        if [ ! -f "./bin/biosim4" ]; then
            ui_error "biosim4 binary not found at ./bin/biosim4"
            exit 1
        fi
        ./bin/biosim4 --preset quick
        ui_check "Quick test completed"
    fi
fi

# Final summary
ui_nl
ui_box_footer_success "Build Complete! 🚀" 40
ui_nl
if [ "$BUILD_BINARIES" = true ]; then
    ui_subsection "Binaries"
    # Show full absolute paths for easy copying (we're already in BUILD_DIR at this point)
    MAIN_EXE_PATH="$(pwd)/bin/biosim4"
    ui_keyval "Main executable" "$MAIN_EXE_PATH"

    if [ -z "$TARGET" ]; then
        TEST_DIR_PATH="$(pwd)/bin"
        ui_keyval "Test binaries" "$TEST_DIR_PATH/test_*"
    elif [ -n "$TARGET" ]; then
        # Show specific target if built
        if [ -f "bin/$TARGET" ]; then
            TARGET_PATH="$(pwd)/bin/$TARGET"
            ui_keyval "Target binary" "$TARGET_PATH"
        fi
    fi
    ui_nl
    ui_subsection "Run Command"
    echo "  $MAIN_EXE_PATH $PROJECT_ROOT/config/biosim4.toml"
fi

if [ "$BUILD_DOCS" = true ] && [ -d "docs/html" ]; then
    ui_nl
    ui_subsection "Documentation"
    DOCS_PATH="$(pwd)/docs/html/index.html"
    ui_keyval "HTML docs" "$DOCS_PATH"
    ui_nl
    ui_subsection "Open Documentation"
    echo "  open $DOCS_PATH"
    ui_nl
fi
