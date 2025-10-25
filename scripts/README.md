# Scripts Directory

This directory contains utility scripts for the biosim4 project.

## Available Scripts

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
