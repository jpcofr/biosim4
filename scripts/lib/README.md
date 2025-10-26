# BioSim4 Script UI Library

Professional bash UI/UX library providing consistent look and feel across all BioSim4 scripts.

## Overview

The UI library (`scripts/lib/ui.sh`) provides a comprehensive set of functions for creating consistent, professional-looking terminal output with:

- **Consistent color scheme** across all scripts
- **Box drawing** for headers and footers
- **Interactive prompts** with standardized behavior
- **Status indicators** (checkmarks, crosses, bullets)
- **Formatted output** (key-value pairs, sections, tables)
- **Error handling** with helpful suggestions

## Quick Start

```bash
#!/bin/bash
# Load the UI library
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/lib/ui.sh"

# Use UI functions
ui_box_header "My Script Title" 40
ui_nl

ui_step "Performing operation..."
# ... do work ...
ui_check "Operation completed"

ui_nl
ui_box_footer_success "✅ All done!" 40
```

## Color Scheme

| Color  | Variable    | Usage                  |
| ------ | ----------- | ---------------------- |
| Red    | `UI_RED`    | Errors, failures       |
| Green  | `UI_GREEN`  | Success, completions   |
| Yellow | `UI_YELLOW` | Warnings, prompts      |
| Blue   | `UI_BLUE`   | Information, headers   |
| Cyan   | `UI_CYAN`   | Highlights, steps      |
| Gray   | `UI_GRAY`   | Separators, muted text |

All colors automatically reset to `UI_NC` (no color) after use.

## Function Reference

### Box Drawing

#### `ui_box_header "Title" [width]`
Creates a boxed header with centered title.

```bash
ui_box_header "BioSim4 Build Script" 40
# Output:
# ╔════════════════════════════════════════╗
# ║     BioSim4 Build Script               ║
# ╚════════════════════════════════════════╝
```

#### `ui_box_footer_success "Message" [width]`
Creates a green success footer box.

```bash
ui_box_footer_success "✅ Build Complete!" 40
```

#### `ui_box_footer_error "Message" [width]`
Creates a red error footer box.

```bash
ui_box_footer_error "❌ Build Failed" 40
```

### Messages

#### `ui_error "Message"`
Prints error message in red (to stderr).

```bash
ui_error "File not found"
```

#### `ui_warn "Message"`
Prints warning message in yellow (to stderr).

```bash
ui_warn "Configuration file missing, using defaults"
```

#### `ui_info "Message"`
Prints informational message in blue.

```bash
ui_info "Processing files..."
```

#### `ui_success "Message"`
Prints success message in green.

```bash
ui_success "✓ All tests passed"
```

#### `ui_highlight "Message"`
Prints highlighted message in cyan.

```bash
ui_highlight "Method: AddressSanitizer (ASan)"
```

### Progress Indicators

#### `ui_step "Description"`
Indicates a step in progress (▶️ icon).

```bash
ui_step "Compiling source files..."
```

#### `ui_bullet "Item"`
Prints a bullet point item.

```bash
ui_bullet "main.cpp"
ui_bullet "utils.cpp"
```

#### `ui_check "Item"`
Prints an item with checkmark (✓).

```bash
ui_check "Build completed successfully"
```

#### `ui_cross "Item"`
Prints an item with cross mark (✗).

```bash
ui_cross "Optional feature disabled"
```

#### Thematic Icons

```bash
ui_clean "Cleaning build directory..."  # 🧹
ui_inspect "Verifying output..."        # 🔍
ui_config "Loading configuration..."    # ⚙️
ui_launch "Starting application..."     # 🚀
ui_video "Generating video..."          # 🎬
```

### Interactive Functions

#### `ui_confirm "Question" [default_yes]`
Asks yes/no question, returns 0 for yes, 1 for no.

```bash
if ui_confirm "Continue with build?" true; then
    echo "Building..."
else
    echo "Cancelled"
fi

# With default No:
if ui_confirm "Delete files?" false; then
    rm -rf temp/
fi
```

#### `ui_input "Prompt" [default]`
Gets input from user with optional default value.

```bash
name=$(ui_input "Enter name: " "default_name")
port=$(ui_input "Enter port: ")
```

#### `ui_progress "Message"`
Displays animated spinner (background process).

```bash
ui_progress "Building..." &
SPINNER_PID=$!
# ... do long-running work ...
kill $SPINNER_PID 2>/dev/null
```

### Validation Functions

#### `ui_require_command "cmd" "install_hint"`
Checks if command exists, shows installation hint if not.

```bash
if ! ui_require_command "cmake" "brew install cmake"; then
    exit 1
fi
```

#### `ui_require_file "path" "hint"`
Checks if file exists.

```bash
if ! ui_require_file "config.toml" "Run: cp config.example.toml config.toml"; then
    exit 1
fi
```

#### `ui_require_dir "path" "hint"`
Checks if directory exists.

```bash
if ! ui_require_dir "build" "Run: mkdir build && cd build && cmake .."; then
    exit 1
fi
```

### Section Functions

#### `ui_section "Title"`
Prints a major section header.

```bash
ui_section "Build Configuration"
# Output:
# ═══════════════════════════════════════
#   Build Configuration
# ═══════════════════════════════════════
```

#### `ui_subsection "Title"`
Prints a subsection header.

```bash
ui_subsection "Environment Variables"
# Output:
# ▶ Environment Variables
```

### Table Functions

#### `ui_keyval "Key" "Value" [indent]`
Prints a key-value pair with alignment.

```bash
ui_keyval "Build Type" "Debug"
ui_keyval "Compiler" "clang++"
# Output:
#   Build Type:               Debug
#   Compiler:                 clang++
```

#### `ui_keyval_color "Key" "Value" [color] [indent]`
Prints a colored key-value pair.

```bash
ui_keyval_color "Status" "Running" "$UI_GREEN"
```

### Error Handling

#### `ui_die "Message" [exit_code]`
Prints error and exits with code (default: 1).

```bash
ui_die "Critical error occurred" 1
```

#### `ui_error_suggest "Error" "Suggestion"`
Prints error with helpful suggestion.

```bash
ui_error_suggest "Build failed" "Try running: ./scripts/build.sh -c"
```

### Utility Functions

#### `ui_separator [character]`
Prints a horizontal separator line.

```bash
ui_separator      # Default: ----
ui_separator "="  # Custom: ====
```

#### `ui_nl [count]`
Prints empty line(s).

```bash
ui_nl      # One blank line
ui_nl 3    # Three blank lines
```

#### `ui_center "Text" [width]`
Centers text within specified width.

```bash
ui_center "BioSim4" 80
```

### Help Formatting

#### `ui_help_section "Title"`
Formats help section headers.

```bash
ui_help_section "Options"
```

#### `ui_help_option "flag" "description"`
Formats help option entries.

```bash
ui_help_option "-r, --release" "Build in release mode"
ui_help_option "-v, --verbose" "Verbose output"
```

#### `ui_help_example "command" "description"`
Formats help examples.

```bash
ui_help_example "./build.sh -r" "Release build"
ui_help_example "./build.sh -c -r" "Clean release build"
```

## Complete Example Script

```bash
#!/bin/bash
set -e

# Load UI library
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/lib/ui.sh"

# Show header
ui_box_header "🎯 My Build Script" 40
ui_nl

# Validate requirements
if ! ui_require_command "cmake" "brew install cmake"; then
    exit 1
fi

# Interactive prompt
if ! ui_confirm "Start build?" true; then
    ui_warn "Build cancelled"
    exit 0
fi

# Show configuration
ui_section "Build Configuration"
ui_keyval "Build Type" "Debug"
ui_keyval "Generator" "Ninja"
ui_keyval "Jobs" "8"
ui_nl

# Perform build steps
ui_step "Configuring CMake..."
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug ..

ui_step "Building binaries..."
ninja -j8
ui_check "Build completed"

ui_nl

# Show results
ui_box_footer_success "✅ Build Successful!" 40
ui_nl

ui_subsection "Output"
ui_keyval "Binary" "./build/bin/myapp"
ui_nl

ui_info "Run: ./build/bin/myapp"
```

## Design Principles

1. **Consistency**: All scripts use the same color scheme and formatting
2. **Readability**: Clear visual hierarchy with boxes, sections, and bullets
3. **User-friendly**: Interactive prompts with sensible defaults
4. **Professional**: Clean output suitable for both interactive and CI/CD use
5. **Maintainable**: Centralized UI logic makes updates easy

## Migration Guide

### Before (Old Style)

```bash
echo -e "${GREEN}Starting build...${NC}"
echo ""
if [ ! -f "$FILE" ]; then
    echo -e "${RED}Error: File not found${NC}"
    exit 1
fi
echo -e "  ${BLUE}•${NC} Building..."
echo -e "${GREEN}✓ Done${NC}"
```

### After (New Style)

```bash
ui_success "Starting build..."
ui_nl
if ! ui_require_file "$FILE" "Create file first"; then
    exit 1
fi
ui_bullet "Building..."
ui_check "Done"
```

## All Refactored Scripts

The following scripts now use the UI library:

- ✅ `build.sh` - Main build script
- ✅ `quick-test.sh` - Quick functionality test
- ✅ `test-video.sh` - Video generation test
- ✅ `test-leaks.sh` - Memory leak testing
- ✅ `run-hooks.sh` - Pre-commit hooks runner

## Benefits

### For Developers
- **Faster script writing** - reusable components
- **Consistent UX** - users know what to expect
- **Less boilerplate** - no more color code duplication
- **Better error handling** - standardized patterns

### For Users
- **Professional appearance** - polished terminal output
- **Clear visual hierarchy** - easy to scan output
- **Helpful prompts** - standardized yes/no questions
- **Better error messages** - contextual help included

## Future Enhancements

Potential additions to the library:

- Progress bars for long operations
- Multi-column table formatting
- More sophisticated menu systems
- Logging to file alongside terminal output
- Theme support (dark/light modes)
- Emoji support toggle for environments that don't support them

## Contributing

When adding new UI functions:

1. Follow the naming convention: `ui_<category>_<action>`
2. Add documentation in this README
3. Export the function at the end of `ui.sh`
4. Keep color usage consistent with existing patterns
5. Test on both macOS and Linux if possible

## See Also

- [Build Quick Reference](BUILD_QUICK_REFERENCE.md) - Build script usage
- [Pre-commit Quickstart](../doc/PRECOMMIT_QUICKSTART.md) - Git hooks setup
- [Scripts README](README.md) - Overview of all scripts
