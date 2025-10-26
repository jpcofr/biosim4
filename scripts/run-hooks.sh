#!/bin/bash
# Run pre-commit hooks manually
# Useful for checking code before committing or in CI/CD pipelines

set -e

# Load UI library
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/lib/ui.sh"

PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Options
RUN_ALL=false
HOOK_STAGE="commit"
SHOW_DIFF=false
VERBOSE=false
FILES=()

show_help() {
    ui_success "BioSim4 Git Hooks Runner"
    echo "Run pre-commit hooks manually for code quality checks"
    ui_nl
    ui_help_section "Usage"
    echo "  $0 [OPTIONS] [FILES...]"
    ui_nl
    ui_help_section "Options"
    ui_help_option "-a, --all" "Run on all files (not just staged)"
    ui_help_option "-s, --stage STAGE" "Run hooks for specific stage (default: commit)"
    echo "                         Stages: commit, push, manual"
    ui_help_option "-d, --diff" "Show diff of changes after running"
    ui_help_option "-v, --verbose" "Verbose output"
    ui_help_option "-h, --help" "Show this help message"
    ui_nl
    ui_help_section "Examples"
    ui_help_example "$0" "Run on staged files"
    ui_help_example "$0 -a" "Run on all files"
    ui_help_example "$0 src/main.cpp" "Run on specific file"
    ui_help_example "$0 -a -d" "Run on all + show changes"
    ui_help_example "$0 --stage manual" "Run manual-stage hooks only"
    ui_nl
    ui_help_section "Hook Stages"
    ui_help_option "commit" "Hooks that run on 'git commit' (default)"
    ui_help_option "push" "Hooks that run on 'git push'"
    ui_help_option "manual" "Hooks configured to run manually only"
    ui_nl
    ui_help_section "Common Workflows"
    ui_keyval "Before committing" "$0"
    ui_keyval "Full project check" "$0 -a"
    ui_keyval "Check specific files" "$0 src/main.cpp include/params.h"
    ui_keyval "Run manual hooks" "$0 --stage manual -a"
    ui_nl
    ui_help_section "What This Runs"
    ui_bullet "C++ formatting (clang-format)"
    ui_bullet "Python formatting (black)"
    ui_bullet "Python import sorting (isort)"
    ui_bullet "Python linting (flake8)"
    ui_bullet "Trailing whitespace removal"
    ui_bullet "End-of-file fixes"
    ui_bullet "YAML validation"
    ui_nl
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
if ! ui_require_command "pre-commit" "Install it with: pip install pre-commit (or) brew install pre-commit"; then
    ui_nl
    echo "Then install hooks:"
    echo "  pre-commit install"
    ui_nl
    echo "See doc/PRECOMMIT_QUICKSTART.md for details"
    exit 1
fi

# Check if hooks are installed
if [ ! -f ".git/hooks/pre-commit" ]; then
    ui_warn "Pre-commit hooks not installed in this repository"
    ui_nl
    if ui_confirm "Install hooks now?" true; then
        ui_step "Installing pre-commit hooks..."
        pre-commit install
        ui_check "Hooks installed"
        ui_nl
    else
        echo "To install later, run: pre-commit install"
        ui_nl
    fi
fi

# Display header
ui_box_header "BioSim4 Git Hooks Runner" 40
ui_nl

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
    ui_highlight "Running hooks on specific files..."
    for file in "${FILES[@]}"; do
        ui_bullet "$file"
    done
    CMD="$CMD --files ${FILES[*]}"
elif [ "$RUN_ALL" = true ]; then
    ui_highlight "Running hooks on all files..."
    CMD="$CMD --all-files"
else
    ui_highlight "Running hooks on staged files..."
fi

ui_nl
ui_keyval "Command" "$CMD"
ui_nl

# Run pre-commit
if eval "$CMD"; then
    ui_nl
    ui_box_footer_success "All Hooks Passed! ✓" 40

    # Show diff if requested
    if [ "$SHOW_DIFF" = true ]; then
        ui_nl
        ui_info "Changes made by hooks:"
        if git diff --color=always | head -100; then
            ui_nl
            ui_warn "(Showing first 100 lines of diff)"
        fi
    fi

    exit 0
else
    ui_nl
    ui_box_footer_error "Some Hooks Failed ✗" 40
    ui_nl
    ui_warn "The hooks may have automatically fixed some issues."
    ui_warn "Review the changes and stage them:"
    ui_nl
    echo "  git diff              # Review changes"
    echo "  git add <files>       # Stage fixes"
    echo "  $0                    # Run hooks again"
    ui_nl
    exit 1
fi
