# Scripts Directory

This directory contains utility scripts for the biosim4 project.

## Available Scripts

### `build.sh` ⭐ NEW

Comprehensive build script with flexible options for different development workflows.

**Usage:**
```bash
./scripts/build.sh [OPTIONS]
```

**Common Use Cases:**
```bash
# Quick development build (default)
./scripts/build.sh

# Release build
./scripts/build.sh -r

# Clean debug build with tests
./scripts/build.sh -c --test

# Build with memory leak detection
./scripts/build.sh -d --sanitizers

# Build specific test
./scripts/build.sh --target test_basic_types

# Build documentation only
./scripts/build.sh --docs-only

# Build everything
./scripts/build.sh -a -r

# Show current configuration
./scripts/build.sh --info

# Dry run (see what would happen)
./scripts/build.sh -r --dry-run
```

**Key Features:**
- **Build modes**: Debug (default) or Release (`-r`)
- **Selective targets**: Build binaries only (default), docs only (`--docs-only`), or both (`-a`)
- **Build systems**: Ninja (default) or Make (`-m`)
- **Sanitizers**: AddressSanitizer (`--sanitizers`) or ThreadSanitizer (`--thread-sanitizer`)
- **Clean builds**: Selective (`-c`) or full (`--full-clean`)
- **Parallel builds**: Auto-detects CPU cores, override with `-j N`
- **Testing**: Build and run tests with `--test`
- **Info mode**: Show configuration without building (`-i`)
- **Verbose output**: See detailed build commands (`-v`)

**Options Reference:**
- `-r, --release` - Release mode (optimizations enabled)
- `-d, --debug` - Debug mode (default)
- `-c, --clean` - Clean build (preserves dependencies)
- `--full-clean` - Remove entire build directory
- `-D, --docs-only` - Build Doxygen documentation only
- `-a, --all` - Build binaries and documentation
- `-t, --target NAME` - Build specific target
- `-j, --jobs N` - Parallel build jobs
- `--no-video` - Disable video generation support
- `--sanitizers` - Enable memory leak detection
- `--thread-sanitizer` - Enable race condition detection
- `--test` - Run tests after building
- `-i, --info` - Show configuration and exit
- `-v, --verbose` - Verbose build output
- `--dry-run` - Preview commands without executing

**Examples by Workflow:**

*Development cycle:*
```bash
./scripts/build.sh              # Quick debug build
./scripts/build.sh --test       # Build and verify with tests
```

*Production builds:*
```bash
./scripts/build.sh -r           # Release build
./scripts/build.sh -c -r        # Clean release build
```

*Memory debugging:*
```bash
./scripts/build.sh --sanitizers           # Memory leaks
./scripts/build.sh --thread-sanitizer     # Race conditions
```

*Documentation:*
```bash
./scripts/build.sh --docs-only            # Build docs
./scripts/build.sh -a                     # Build everything
```

*Fresh start:*
```bash
./scripts/build.sh --full-clean -d        # Nuclear option
```

### `run-hooks.sh` ⭐ NEW

Run pre-commit hooks manually for code quality checks before committing.

**Usage:**
```bash
./scripts/run-hooks.sh [OPTIONS] [FILES...]
```

**Common Use Cases:**
```bash
# Run on staged files (before commit)
./scripts/run-hooks.sh

# Run on all files
./scripts/run-hooks.sh -a

# Run on specific files
./scripts/run-hooks.sh src/main.cpp include/params.h

# Run and show changes made
./scripts/run-hooks.sh -a -d

# Run manual-stage hooks only
./scripts/run-hooks.sh --stage manual -a
```

**Key Features:**
- **Auto-run on commit**: Hooks run automatically on `git commit` (if installed)
- **Manual execution**: Run hooks anytime without committing
- **Selective scope**: Run on staged files, all files, or specific files
- **Multiple stages**: commit, push, or manual hooks
- **Auto-fix**: Many hooks automatically fix issues (formatting, whitespace, etc.)

**What This Runs:**
- C++ formatting (clang-format)
- Python formatting (black)
- Python import sorting (isort)
- Python linting (flake8)
- Trailing whitespace removal
- End-of-file fixes
- YAML validation
- Large file detection
- Merge conflict detection

**Options:**
- `-a, --all` - Run on all files (not just staged)
- `-s, --stage STAGE` - Run hooks for specific stage (commit, push, manual)
- `-d, --diff` - Show diff of changes after running
- `-v, --verbose` - Verbose output
- `-h, --help` - Show help message

**First-Time Setup:**
```bash
# Install pre-commit (one-time)
pip install pre-commit

# Install hooks in repo (one-time)
pre-commit install
```

**Common Workflows:**
```bash
# Before committing
./scripts/run-hooks.sh

# Full project check
./scripts/run-hooks.sh -a

# Check specific changes
./scripts/run-hooks.sh src/main.cpp src/params.cpp
```

See [`doc/PRECOMMIT_QUICKSTART.md`](../doc/PRECOMMIT_QUICKSTART.md) for detailed setup instructions.

### `clean-build.sh`

Selectively cleans the build directory while preserving time-consuming OpenCV build artifacts.

**Usage:**
```bash
./scripts/clean-build.sh
```

**What it does:**
- Removes CMake cache and configuration files
- Removes build system files (Makefile, build.ninja, etc.)
- Removes biosim4 build artifacts
- **Preserves** OpenCV build artifacts (saves 30-60 minutes of rebuild time)

**When to use:**
- After changing CMake configuration
- When experiencing build issues
- Before committing to ensure clean build state
- When switching between build configurations

**What it preserves:**
- `_deps/opencv-*` directories (if using FetchContent)
- OpenCV-related CMakeFiles

**After cleaning:**
```bash
cd build
cmake -G Ninja ..
ninja
```

### `test-leaks.sh`

Memory leak testing script for biosim4.

**Usage:**
```bash
./scripts/test-leaks.sh
```

See [`tests/docs/LEAK_TEST_QUICKSTART.md`](../tests/docs/LEAK_TEST_QUICKSTART.md) for details.
