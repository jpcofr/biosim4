# Quick Start: Memory Leak Testing

## TL;DR - Fastest Method

```bash
# 1. Run the automated test script
./scripts/test-leaks.sh

# That's it! ✅
```

## What Just Happened?

The script:
1. ✅ Rebuilds your project with AddressSanitizer (ASan) enabled
2. ✅ Runs a minimal 5-generation simulation (takes ~30 seconds)
3. ✅ Automatically detects and reports any memory leaks
4. ✅ Shows you exactly where leaks occurred with stack traces

## Example Output

### ✅ No Leaks (Success)
```
🔍 BioSim4 Memory Leak Testing
================================

Method: AddressSanitizer (ASan)

Running simulator with leak detection...
Config: tests/configs/leak-test.ini

Gen 1, 45 survivors
Gen 2, 52 survivors
Gen 3, 48 survivors
Gen 4, 51 survivors
Gen 5, 49 survivors

✅ No memory leaks detected!
```

### ❌ Leaks Found
```
=================================================================
==12345==ERROR: LeakSanitizer: detected memory leaks

Direct leak of 1024 bytes in 1 object(s) allocated from:
    #0 0x1045a2b40 in operator new(unsigned long)
    #1 0x104530abc in BS::ImageWriter::initVideo() imageWriter.cpp:156
    #2 0x104532def in BS::simulator() simulator.cpp:89
    
SUMMARY: AddressSanitizer: 1024 byte(s) leaked in 1 allocation(s).

❌ Memory leaks or errors found. See output above.
```

## Other Methods

### macOS Native Tools (No Rebuild Required)

```bash
# Simple leaks command
./scripts/test-leaks.sh leaks

# Xcode Instruments (visual GUI)
./scripts/test-leaks.sh instruments
```

## Manual Testing

### Build Once with ASan

```bash
rm -rf build/
mkdir build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON ..
ninja
cd ..
```

### Run Your Own Tests

```bash
# Short test
./build/bin/biosim4 tests/configs/leak-test.ini

# Full simulation
./build/bin/biosim4 biosim4.ini

# Custom config
./build/bin/biosim4 my-config.ini
```

### Advanced ASan Options

```bash
# More verbose output
ASAN_OPTIONS=verbosity=1 ./build/bin/biosim4 biosim4.ini

# Check at specific points (not just exit)
ASAN_OPTIONS=detect_leaks=1:leak_check_at_exit=1 ./build/bin/biosim4

# Suppress known false positives
LSAN_OPTIONS=suppressions=lsan.supp ./build/bin/biosim4
```

## When to Test for Leaks

✅ **DO test:**
- After adding new features
- Before merging PRs
- When refactoring memory management
- If you notice memory usage growing over time
- Before releases

❌ **DON'T need to test:**
- Every single commit (too slow)
- For minor documentation changes
- For UI/config-only changes

## Performance Impact

| Method | Build Time | Run Time | Overhead |
|--------|------------|----------|----------|
| ASan | +10-20% | +5 sec | ~2x slower |
| Leaks | No rebuild | +30 sec | ~10x slower |
| Instruments | No rebuild | GUI | ~5x slower |

**Recommendation:** Use ASan for regular development, Instruments for visual analysis.

## CI/CD Integration

The script can be used in GitHub Actions:

```yaml
- name: Memory Leak Check
  run: ./scripts/test-leaks.sh
```

(Full CI/CD example in `doc/MEMORY_LEAK_TESTING.md`)

## Common Issues

### "Binary not found"
The script automatically builds it. Just wait ~30 seconds.

### "Config file not found"
Make sure you run from the project root:
```bash
cd /path/to/biosim4
./scripts/test-leaks.sh
```

### False Positives from OpenCV
Create `lsan.supp`:
```
leak:libopencv_core
leak:libopencv_videoio
```

Then run:
```bash
LSAN_OPTIONS=suppressions=lsan.supp ./build/bin/biosim4
```

## Need More Detail?

See the comprehensive guide:
📚 [doc/MEMORY_LEAK_TESTING.md](doc/MEMORY_LEAK_TESTING.md)

## Summary Commands

```bash
# Quick test (30 seconds)
./scripts/test-leaks.sh

# Visual analysis (macOS)
./scripts/test-leaks.sh instruments

# Full test with your config
rm -rf build && mkdir build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON ..
ninja
cd .. && ./build/bin/biosim4 biosim4.ini
```
