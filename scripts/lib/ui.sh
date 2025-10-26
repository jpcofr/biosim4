#!/bin/bash
# BioSim4 Script UI Library
# Common UI/UX functions for consistent user interaction across all scripts
#
# Usage: source "$(dirname "$0")/lib/ui.sh"

# ============================================================================
# COLOR DEFINITIONS
# ============================================================================

# ANSI color codes for consistent terminal output
readonly UI_RED='\033[0;31m'
readonly UI_GREEN='\033[0;32m'
readonly UI_YELLOW='\033[1;33m'
readonly UI_BLUE='\033[0;34m'
readonly UI_CYAN='\033[0;36m'
readonly UI_MAGENTA='\033[0;35m'
readonly UI_WHITE='\033[1;37m'
readonly UI_GRAY='\033[0;90m'
readonly UI_NC='\033[0m'  # No Color

# ============================================================================
# BOX DRAWING CHARACTERS
# ============================================================================

readonly UI_BOX_TL='╔'  # Top Left
readonly UI_BOX_TR='╗'  # Top Right
readonly UI_BOX_BL='╚'  # Bottom Left
readonly UI_BOX_BR='╝'  # Bottom Right
readonly UI_BOX_H='═'   # Horizontal
readonly UI_BOX_V='║'   # Vertical

# ============================================================================
# FORMATTING FUNCTIONS
# ============================================================================

# Print a horizontal line of specified length
# Usage: ui_line [length] [character]
ui_line() {
    local length="${1:-40}"
    local char="${2:-${UI_BOX_H}}"
    printf "%${length}s" | tr ' ' "$char"
}

# Print a box header with title
# Usage: ui_box_header "Title" [width]
ui_box_header() {
    local title="$1"
    local width="${2:-40}"
    local padding=$(( (width - ${#title} - 4) / 2 ))

    echo -e "${UI_BLUE}${UI_BOX_TL}$(ui_line $width)${UI_BOX_TR}${UI_NC}"
    printf "${UI_BLUE}${UI_BOX_V}${UI_NC}"
    printf "%${padding}s" ""
    printf "${UI_BLUE}  %s  ${UI_NC}" "$title"
    printf "%${padding}s" ""
    printf "${UI_BLUE}${UI_BOX_V}${UI_NC}\n"
    echo -e "${UI_BLUE}${UI_BOX_BL}$(ui_line $width)${UI_BOX_BR}${UI_NC}"
}

# Print a box footer (success)
# Usage: ui_box_footer_success "Message" [width]
ui_box_footer_success() {
    local message="$1"
    local width="${2:-40}"
    local padding=$(( (width - ${#message} - 4) / 2 ))

    echo -e "${UI_GREEN}${UI_BOX_TL}$(ui_line $width)${UI_BOX_TR}${UI_NC}"
    printf "${UI_GREEN}${UI_BOX_V}${UI_NC}"
    printf "%${padding}s" ""
    printf "${UI_GREEN}  %s  ${UI_NC}" "$message"
    printf "%${padding}s" ""
    printf "${UI_GREEN}${UI_BOX_V}${UI_NC}\n"
    echo -e "${UI_GREEN}${UI_BOX_BL}$(ui_line $width)${UI_BOX_BR}${UI_NC}"
}

# Print a box footer (error)
# Usage: ui_box_footer_error "Message" [width]
ui_box_footer_error() {
    local message="$1"
    local width="${2:-40}"
    local padding=$(( (width - ${#message} - 4) / 2 ))

    echo -e "${UI_RED}${UI_BOX_TL}$(ui_line $width)${UI_BOX_TR}${UI_NC}"
    printf "${UI_RED}${UI_BOX_V}${UI_NC}"
    printf "%${padding}s" ""
    printf "${UI_RED}  %s  ${UI_NC}" "$message"
    printf "%${padding}s" ""
    printf "${UI_RED}${UI_BOX_V}${UI_NC}\n"
    echo -e "${UI_RED}${UI_BOX_BL}$(ui_line $width)${UI_BOX_BR}${UI_NC}"
}

# ============================================================================
# MESSAGE FUNCTIONS
# ============================================================================

# Print an error message
# Usage: ui_error "Message"
ui_error() {
    echo -e "${UI_RED}Error: $1${UI_NC}" >&2
}

# Print a warning message
# Usage: ui_warn "Message"
ui_warn() {
    echo -e "${UI_YELLOW}Warning: $1${UI_NC}" >&2
}

# Print an info message
# Usage: ui_info "Message"
ui_info() {
    echo -e "${UI_BLUE}$1${UI_NC}"
}

# Print a success message
# Usage: ui_success "Message"
ui_success() {
    echo -e "${UI_GREEN}$1${UI_NC}"
}

# Print a highlighted message
# Usage: ui_highlight "Message"
ui_highlight() {
    echo -e "${UI_CYAN}$1${UI_NC}"
}

# Print a step indicator
# Usage: ui_step "Step description"
ui_step() {
    echo -e "${UI_CYAN}▶️  $1${UI_NC}"
}

# Print a bullet point
# Usage: ui_bullet "Item"
ui_bullet() {
    echo -e "  ${UI_BLUE}•${UI_NC} $1"
}

# Print a checkmark item
# Usage: ui_check "Item"
ui_check() {
    echo -e "  ${UI_GREEN}✓${UI_NC} $1"
}

# Print a cross item
# Usage: ui_cross "Item"
ui_cross() {
    echo -e "  ${UI_RED}✗${UI_NC} $1"
}

# Print a cleaning/maintenance indicator
# Usage: ui_clean "Description"
ui_clean() {
    echo -e "${UI_CYAN}🧹 $1${UI_NC}"
}

# Print an inspection indicator
# Usage: ui_inspect "Description"
ui_inspect() {
    echo -e "${UI_CYAN}🔍 $1${UI_NC}"
}

# Print a gear/configuration indicator
# Usage: ui_config "Description"
ui_config() {
    echo -e "${UI_BLUE}⚙️  $1${UI_NC}"
}

# Print a rocket/launch indicator
# Usage: ui_launch "Description"
ui_launch() {
    echo -e "${UI_BLUE}🚀 $1${UI_NC}"
}

# Print a video/media indicator
# Usage: ui_video "Description"
ui_video() {
    echo -e "${UI_BLUE}🎬 $1${UI_NC}"
}

# ============================================================================
# INTERACTIVE FUNCTIONS
# ============================================================================

# Ask a yes/no question (returns 0 for yes, 1 for no)
# Usage: if ui_confirm "Proceed?"; then ...
# Options: ui_confirm "Question" [default_yes]
ui_confirm() {
    local question="$1"
    local default_yes="${2:-false}"
    local prompt

    if [ "$default_yes" = true ]; then
        prompt="(Y/n)"
    else
        prompt="(y/N)"
    fi

    while true; do
        read -p "$(echo -e ${UI_YELLOW})${question} ${prompt} $(echo -e ${UI_NC})" -n 1 -r
        echo

        if [[ -z "$REPLY" ]]; then
            # Empty response - use default
            [ "$default_yes" = true ] && return 0 || return 1
        elif [[ $REPLY =~ ^[Yy]$ ]]; then
            return 0
        elif [[ $REPLY =~ ^[Nn]$ ]]; then
            return 1
        else
            ui_warn "Please answer 'y' or 'n'"
        fi
    done
}

# Ask for input with a prompt
# Usage: result=$(ui_input "Enter value: " [default])
ui_input() {
    local prompt="$1"
    local default="$2"
    local result

    if [ -n "$default" ]; then
        read -p "$(echo -e ${UI_CYAN})${prompt}[${default}]: $(echo -e ${UI_NC})" result
        echo "${result:-$default}"
    else
        read -p "$(echo -e ${UI_CYAN})${prompt}$(echo -e ${UI_NC})" result
        echo "$result"
    fi
}

# Display a progress indicator
# Usage: ui_progress "Processing..." &
#        PID=$!
#        # ... do work ...
#        kill $PID 2>/dev/null
ui_progress() {
    local message="$1"
    local spin='⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏'
    local i=0

    while true; do
        i=$(( (i+1) % ${#spin} ))
        printf "\r${UI_CYAN}${spin:$i:1} %s${UI_NC}" "$message"
        sleep 0.1
    done
}

# ============================================================================
# VALIDATION FUNCTIONS
# ============================================================================

# Check if a command exists
# Usage: ui_require_command "cmake" "Install with: brew install cmake"
ui_require_command() {
    local cmd="$1"
    local install_hint="$2"

    if ! command -v "$cmd" &> /dev/null; then
        ui_error "$cmd is not installed"
        if [ -n "$install_hint" ]; then
            echo ""
            echo "$install_hint"
        fi
        return 1
    fi
    return 0
}

# Check if a file exists
# Usage: ui_require_file "config.toml" "Run setup first"
ui_require_file() {
    local file="$1"
    local hint="$2"

    if [ ! -f "$file" ]; then
        ui_error "Required file not found: $file"
        if [ -n "$hint" ]; then
            echo ""
            echo "$hint"
        fi
        return 1
    fi
    return 0
}

# Check if a directory exists
# Usage: ui_require_dir "build" "Run cmake first"
ui_require_dir() {
    local dir="$1"
    local hint="$2"

    if [ ! -d "$dir" ]; then
        ui_error "Required directory not found: $dir"
        if [ -n "$hint" ]; then
            echo ""
            echo "$hint"
        fi
        return 1
    fi
    return 0
}

# ============================================================================
# SECTION FUNCTIONS
# ============================================================================

# Print a section header
# Usage: ui_section "Configuration"
ui_section() {
    echo ""
    echo -e "${UI_BLUE}═══════════════════════════════════════${UI_NC}"
    echo -e "${UI_BLUE}  $1${UI_NC}"
    echo -e "${UI_BLUE}═══════════════════════════════════════${UI_NC}"
    echo ""
}

# Print a subsection header
# Usage: ui_subsection "Build Settings"
ui_subsection() {
    echo ""
    echo -e "${UI_GREEN}▶ $1${UI_NC}"
    echo ""
}

# ============================================================================
# TABLE FUNCTIONS
# ============================================================================

# Print a key-value pair
# Usage: ui_keyval "Key" "Value" [indent]
ui_keyval() {
    local key="$1"
    local value="$2"
    local indent="${3:-  }"

    printf "${indent}%-25s %s\n" "$key:" "$value"
}

# Print a colored key-value pair
# Usage: ui_keyval_color "Key" "Value" [color] [indent]
ui_keyval_color() {
    local key="$1"
    local value="$2"
    local color="${3:-${UI_NC}}"
    local indent="${4:-  }"

    printf "${indent}%-25s ${color}%s${UI_NC}\n" "$key:" "$value"
}

# ============================================================================
# ERROR HANDLING
# ============================================================================

# Exit with error message and code
# Usage: ui_die "Fatal error occurred" [exit_code]
ui_die() {
    local message="$1"
    local code="${2:-1}"

    echo ""
    ui_box_footer_error "$message"
    echo ""
    exit "$code"
}

# Print error with suggestion
# Usage: ui_error_suggest "Build failed" "Try running: make clean"
ui_error_suggest() {
    local error="$1"
    local suggestion="$2"

    ui_error "$error"
    echo ""
    echo -e "${UI_YELLOW}Suggestion:${UI_NC}"
    echo "  $suggestion"
    echo ""
}

# ============================================================================
# UTILITY FUNCTIONS
# ============================================================================

# Print a separator line
# Usage: ui_separator [character]
ui_separator() {
    local char="${1:--}"
    echo -e "${UI_GRAY}$(printf '%*s' 80 '' | tr ' ' "$char")${UI_NC}"
}

# Print an empty line
# Usage: ui_nl [count]
ui_nl() {
    local count="${1:-1}"
    for ((i=0; i<count; i++)); do
        echo ""
    done
}

# Print centered text
# Usage: ui_center "Text" [width]
ui_center() {
    local text="$1"
    local width="${2:-80}"
    local padding=$(( (width - ${#text}) / 2 ))

    printf "%${padding}s%s\n" "" "$text"
}

# ============================================================================
# HELP FORMATTING
# ============================================================================

# Print help section header
# Usage: ui_help_section "Options"
ui_help_section() {
    echo -e "${UI_BLUE}$1:${UI_NC}"
}

# Print help option
# Usage: ui_help_option "-h, --help" "Show help message"
ui_help_option() {
    local option="$1"
    local description="$2"

    printf "  %-30s %s\n" "$option" "$description"
}

# Print help example
# Usage: ui_help_example "script.sh -r" "Release build"
ui_help_example() {
    local command="$1"
    local description="$2"

    if [ -n "$description" ]; then
        printf "  %-40s ${UI_GRAY}# %s${UI_NC}\n" "$command" "$description"
    else
        echo "  $command"
    fi
}

# ============================================================================
# EXPORT FUNCTIONS
# ============================================================================

# Export all UI functions for use in other scripts
export -f ui_line ui_box_header ui_box_footer_success ui_box_footer_error
export -f ui_error ui_warn ui_info ui_success ui_highlight
export -f ui_step ui_bullet ui_check ui_cross
export -f ui_clean ui_inspect ui_config ui_launch ui_video
export -f ui_confirm ui_input ui_progress
export -f ui_require_command ui_require_file ui_require_dir
export -f ui_section ui_subsection
export -f ui_keyval ui_keyval_color
export -f ui_die ui_error_suggest
export -f ui_separator ui_nl ui_center
export -f ui_help_section ui_help_option ui_help_example
