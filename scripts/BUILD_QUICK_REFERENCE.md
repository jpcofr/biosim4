# BioSim4 Build Script - Quick Reference

## 📦 Common Commands

```bash
# Standard builds
./scripts/build.sh              # Debug build (default)
./scripts/build.sh -r           # Release build
./scripts/build.sh -c           # Clean debug build
./scripts/build.sh -c -r        # Clean release build

# Testing
./scripts/build.sh --test                      # Build and run all tests
./scripts/build.sh --target test_basic_types   # Build specific test

# Documentation
./scripts/build.sh --docs-only  # Build docs only
./scripts/build.sh -a           # Build binaries + docs

# Debugging
./scripts/build.sh --sanitizers         # Memory leak detection
./scripts/build.sh --thread-sanitizer   # Race condition detection

# Information
./scripts/build.sh --info       # Show configuration
./scripts/build.sh --help       # Full help
./scripts/build.sh --dry-run    # Preview without building
```

## 🎯 Workflow Examples

### Daily Development
```bash
# Edit code...
./scripts/build.sh              # Quick rebuild
./build/bin/biosim4 config/biosim4.toml
```

### Testing Changes
```bash
./scripts/build.sh --test       # Build and verify
# Or target specific test:
./scripts/build.sh --target test_neural_net_wiring
./build/bin/test_neural_net_wiring
```

### Debugging Memory Issues
```bash
./scripts/build.sh -d --sanitizers
./build/bin/biosim4 config/biosim4.toml
# ASan will report leaks automatically
```

### Debugging Race Conditions
```bash
./scripts/build.sh -d --thread-sanitizer
./build/bin/biosim4 config/biosim4.toml
# TSan will report data races
```

### Preparing for Release
```bash
./scripts/build.sh --full-clean    # Nuclear clean
./scripts/build.sh -r --test       # Release build + tests
./scripts/build.sh -a -r           # + documentation
```

### Documentation Updates
```bash
# Update Doxygen comments in code...
./scripts/build.sh --docs-only
open build/docs/html/index.html
```

### Build Investigation
```bash
./scripts/build.sh --info           # Check current settings
./scripts/build.sh -v               # Verbose build output
./scripts/build.sh -r --dry-run     # See what would happen
```

## ⚙️ Options at a Glance

| Option               | Description                 |
| -------------------- | --------------------------- |
| `-r, --release`      | Release mode (optimized)    |
| `-d, --debug`        | Debug mode (default)        |
| `-c, --clean`        | Clean build (keep deps)     |
| `--full-clean`       | Nuke entire build/          |
| `-a, --all`          | Build binaries + docs       |
| `-D, --docs-only`    | Build docs only             |
| `-t, --target NAME`  | Build specific target       |
| `-j, --jobs N`       | Parallel jobs (auto-detect) |
| `--no-video`         | Disable video support       |
| `--sanitizers`       | AddressSanitizer + UBSan    |
| `--thread-sanitizer` | ThreadSanitizer             |
| `--test`             | Run tests after build       |
| `-i, --info`         | Show config and exit        |
| `-v, --verbose`      | Verbose output              |
| `--dry-run`          | Preview only                |

## 🔧 Build Generators

```bash
./scripts/build.sh -n           # Ninja (default, fastest)
./scripts/build.sh -m           # Unix Makefiles
```

## 🎓 Tips

1. **Fast incremental builds**: Just run `./scripts/build.sh` (no clean needed)
2. **Parallel builds**: Auto-uses all CPU cores (override with `-j N`)
3. **Clean when needed**: Use `-c` for CMake issues, `--full-clean` rarely
4. **Test specific changes**: Use `--target` instead of rebuilding everything
5. **Memory debugging**: Always use `--sanitizers` or `--thread-sanitizer`, not both
6. **Check first**: Use `--info` to verify settings before long builds
7. **Safe testing**: Use `--dry-run` to preview complex build commands

## 🚨 Common Issues

**CMake not found:**
```bash
export PATH="/opt/homebrew/bin:$PATH"
./scripts/build.sh
```

**Build fails after config changes:**
```bash
./scripts/build.sh -c           # Clean and rebuild
```

**Slow builds:**
```bash
./scripts/build.sh -j 16        # More parallel jobs
```

**Sanitizer conflicts:**
```bash
# ERROR: Can't use both sanitizers together
./scripts/build.sh --sanitizers          # Use this OR
./scripts/build.sh --thread-sanitizer    # Use this
```
