# UI Library Quick Reference Card

## Loading the Library

```bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/lib/ui.sh"
```

## Most Common Functions

### Headers & Footers
```bash
ui_box_header "Title" 40           # Boxed header
ui_box_footer_success "✅ Done" 40 # Success box
ui_box_footer_error "❌ Failed" 40 # Error box
```

### Messages
```bash
ui_error "Something failed"        # Red error (stderr)
ui_warn "Careful!"                 # Yellow warning (stderr)
ui_info "FYI: information"         # Blue info
ui_success "It worked!"            # Green success
ui_highlight "Important detail"    # Cyan highlight
```

### Progress Indicators
```bash
ui_step "Doing something..."       # ▶️ Step indicator
ui_bullet "Item"                   # • Bullet point
ui_check "Completed"               # ✓ Checkmark
ui_cross "Skipped"                 # ✗ Cross mark
```

### Thematic Icons
```bash
ui_clean "Cleaning..."             # 🧹
ui_inspect "Checking..."           # 🔍
ui_config "Configuring..."         # ⚙️
ui_launch "Starting..."            # 🚀
ui_video "Recording..."            # 🎬
```

### Interactive
```bash
# Yes/no prompt (returns 0=yes, 1=no)
if ui_confirm "Continue?" true; then
    # User said yes (true = default yes)
fi

# Get input
name=$(ui_input "Your name: " "default")
```

### Validation
```bash
ui_require_command "cmake" "brew install cmake" || exit 1
ui_require_file "config.toml" "Create it first" || exit 1
ui_require_dir "build/" "Run cmake first" || exit 1
```

### Sections
```bash
ui_section "Major Section"         # ═══ header ═══
ui_subsection "Subsection"         # ▶ header
```

### Key-Value Pairs
```bash
ui_keyval "Build Type" "Debug"     # Aligned table
ui_keyval_color "Status" "OK" "$UI_GREEN"
```

### Error Handling
```bash
ui_die "Fatal error" 1             # Print & exit
ui_error_suggest "Failed" "Try: make clean"
```

### Utilities
```bash
ui_nl                              # Blank line
ui_nl 3                            # 3 blank lines
ui_separator                       # --------
ui_separator "="                   # ========
```

### Help Formatting
```bash
ui_help_section "Options"
ui_help_option "-v, --verbose" "Verbose output"
ui_help_example "./script -v" "With verbose"
```

## Color Constants

```bash
UI_RED      # Errors
UI_GREEN    # Success
UI_YELLOW   # Warnings
UI_BLUE     # Info
UI_CYAN     # Highlights
UI_GRAY     # Muted
UI_NC       # No color (reset)
```

## Typical Script Structure

```bash
#!/bin/bash
set -e

# 1. Load library
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/lib/ui.sh"

# 2. Show header
ui_box_header "My Script Title" 40
ui_nl

# 3. Validate requirements
ui_require_command "cmake" "brew install cmake" || exit 1

# 4. Interactive prompt
if ! ui_confirm "Start?" true; then
    ui_warn "Cancelled"
    exit 0
fi

# 5. Show config
ui_section "Configuration"
ui_keyval "Option" "Value"
ui_nl

# 6. Do work with progress
ui_step "Processing..."
# ... work ...
ui_check "Done"

# 7. Show results
ui_nl
ui_box_footer_success "✅ Complete!" 40
ui_nl

ui_subsection "Output"
ui_keyval "Result" "/path/to/output"
```

## Common Patterns

### Check then proceed
```bash
if ! ui_require_file "$FILE" "Create it first"; then
    exit 1
fi
```

### Show status
```bash
ui_step "Building..."
if make; then
    ui_check "Build successful"
else
    ui_cross "Build failed"
    exit 1
fi
```

### List items
```bash
ui_info "Files processed:"
for file in *.txt; do
    ui_bullet "$file"
done
```

### Configuration display
```bash
ui_section "Settings"
ui_keyval "Mode" "$MODE"
ui_keyval "Output" "$OUTPUT_DIR"
ui_keyval "Verbose" "$VERBOSE"
ui_nl
```

### Multiple options help
```bash
ui_help_section "Options"
ui_help_option "-h, --help" "Show help"
ui_help_option "-v, --verbose" "Verbose mode"
ui_nl

ui_help_section "Examples"
ui_help_example "./script -v" "Verbose"
ui_help_example "./script --help" "Help"
```

## Tips

1. **Always use `ui_nl`** instead of `echo ""`
2. **Exit codes**: 0 = success, 1 = error
3. **Errors to stderr**: `ui_error` and `ui_warn` go to stderr
4. **Consistent widths**: Use 40 for narrow, 80 for wide boxes
5. **Color sparingly**: Too many colors = visual noise
6. **Test interactively**: Make sure prompts work as expected

## See Full Docs

`scripts/lib/README.md` - Complete API reference with examples
