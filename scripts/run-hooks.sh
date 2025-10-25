#!/bin/bash
# Run pre-commit hooks manually
# Useful for checking code before committing or in CI/CD pipelines

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Script location
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Options
RUN_ALL=false
HOOK_STAGE="commit"
SHOW_DIFF=false
VERBOSE=false
FILES=()

show_help() {
    echo -e "${GREEN}BioSim4 Git Hooks Runner${NC}"
    echo "Run pre-commit hooks manually for code quality checks"
    echo ""
    echo -e "${BLUE}Usage:${NC}"
    echo "  $0 [OPTIONS] [FILES...]"
    echo ""
    echo -e "${BLUE}Options:${NC}"
    echo "  -a, --all              Run on all files (not just staged)"
    echo "  -s, --stage STAGE      Run hooks for specific stage (default: commit)"
    echo "                         Stages: commit, push, manual"
    echo "  -d, --diff             Show diff of changes after running"
    echo "  -v, --verbose          Verbose output"
    echo "  -h, --help             Show this help message"
    echo ""
    echo -e "${BLUE}Examples:${NC}"
    echo "  $0                                  # Run on staged files"
    echo "  $0 -a                               # Run on all files"
    echo "  $0 src/main.cpp                     # Run on specific file"
    echo "  $0 -a -d                            # Run on all + show changes"
    echo "  $0 --stage manual                   # Run manual-stage hooks only"
    echo ""
    echo -e "${BLUE}Hook Stages:${NC}"
    echo "  commit    - Hooks that run on 'git commit' (default)"
    echo "  push      - Hooks that run on 'git push'"
    echo "  manual    - Hooks configured to run manually only"
    echo ""
    echo -e "${BLUE}Common Workflows:${NC}"
    echo "  Before committing:        $0"
    echo "  Full project check:       $0 -a"
    echo "  Check specific files:     $0 src/main.cpp include/params.h"
    echo "  Run manual hooks:         $0 --stage manual -a"
    echo ""
    echo -e "${BLUE}What This Runs:${NC}"
    echo "  • C++ formatting (clang-format)"
    echo "  • Python formatting (black)"
    echo "  • Python import sorting (isort)"
    echo "  • Python linting (flake8)"
    echo "  • Trailing whitespace removal"
    echo "  • End-of-file fixes"
    echo "  • YAML validation"
    echo ""
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -a|--all)
            RUN_ALL=true
            shift
            ;;
        -s|--stage)
            HOOK_STAGE="$2"
            shift 2
            ;;
        -d|--diff)
            SHOW_DIFF=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        -*)
            echo -e "${RED}Unknown option: $1${NC}"
            show_help
            exit 1
            ;;
        *)
            FILES+=("$1")
            shift
            ;;
    esac
done

# Navigate to project root
cd "$PROJECT_ROOT"

# Check if pre-commit is installed
if ! command -v pre-commit &> /dev/null; then
    echo -e "${RED}Error: pre-commit is not installed${NC}"
    echo ""
    echo "Install it with:"
    echo "  pip install pre-commit"
    echo "  # or"
    echo "  brew install pre-commit"
    echo ""
    echo "Then install hooks:"
    echo "  pre-commit install"
    echo ""
    echo "See doc/PRECOMMIT_QUICKSTART.md for details"
    exit 1
fi

# Check if hooks are installed
if [ ! -f ".git/hooks/pre-commit" ]; then
    echo -e "${YELLOW}Warning: Pre-commit hooks not installed in this repository${NC}"
    echo ""
    read -p "Install hooks now? (Y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Nn]$ ]]; then
        echo -e "${BLUE}Installing pre-commit hooks...${NC}"
        pre-commit install
        echo -e "${GREEN}✓ Hooks installed${NC}"
        echo ""
    else
        echo "To install later, run: pre-commit install"
        echo ""
    fi
fi

# Display header
echo -e "${BLUE}╔══════════════════════════════════════╗${NC}"
echo -e "${BLUE}║     BioSim4 Git Hooks Runner         ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════╝${NC}"
echo ""

# Build command
CMD="pre-commit run"

# Add stage
if [ "$HOOK_STAGE" != "commit" ]; then
    CMD="$CMD --hook-stage $HOOK_STAGE"
fi

# Add verbose flag
if [ "$VERBOSE" = true ]; then
    CMD="$CMD --verbose"
fi

# Determine scope
if [ ${#FILES[@]} -gt 0 ]; then
    echo -e "${CYAN}Running hooks on specific files...${NC}"
    for file in "${FILES[@]}"; do
        echo "  • $file"
    done
    CMD="$CMD --files ${FILES[*]}"
elif [ "$RUN_ALL" = true ]; then
    echo -e "${CYAN}Running hooks on all files...${NC}"
    CMD="$CMD --all-files"
else
    echo -e "${CYAN}Running hooks on staged files...${NC}"
fi

echo ""
echo -e "${BLUE}Command:${NC} $CMD"
echo ""

# Run pre-commit
if eval "$CMD"; then
    echo ""
    echo -e "${GREEN}╔══════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║       All Hooks Passed! ✓            ║${NC}"
    echo -e "${GREEN}╚══════════════════════════════════════╝${NC}"

    # Show diff if requested
    if [ "$SHOW_DIFF" = true ]; then
        echo ""
        echo -e "${BLUE}Changes made by hooks:${NC}"
        if git diff --color=always | head -100; then
            echo ""
            echo -e "${YELLOW}(Showing first 100 lines of diff)${NC}"
        fi
    fi

    exit 0
else
    echo ""
    echo -e "${RED}╔══════════════════════════════════════╗${NC}"
    echo -e "${RED}║       Some Hooks Failed ✗            ║${NC}"
    echo -e "${RED}╚══════════════════════════════════════╝${NC}"
    echo ""
    echo -e "${YELLOW}The hooks may have automatically fixed some issues.${NC}"
    echo -e "${YELLOW}Review the changes and stage them:${NC}"
    echo ""
    echo "  git diff              # Review changes"
    echo "  git add <files>       # Stage fixes"
    echo "  $0                    # Run hooks again"
    echo ""
    exit 1
fi
