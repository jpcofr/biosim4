# BioSim4 Test Suite - Google Test

This directory contains the **Google Test** based test suite for BioSim4 C++ components.

## Directory Structure

```
tests/
├── CMakeLists.txt          # Test build configuration
├── README_GTEST.md         # This file
├── unit/                   # Unit tests
│   ├── test_basic_types.cpp
│   ├── test_neural_net_wiring.cpp
│   └── test_grid_visit_neighborhood.cpp
├── integration/            # Integration tests (future)
└── fixtures/               # Test fixtures and data (future)
```

## Running Tests

### Build and Run All Tests

```bash
cd build
cmake -G Ninja ..
ninja
ctest --output-on-failure
```

Or use the custom target:
```bash
ninja run_tests
```

### Run Individual Tests

Test executables are built to `build/bin/`:

```bash
./build/bin/test_basic_types
./build/bin/test_neural_net_wiring
./build/bin/test_grid_visit_neighborhood
```

### Run Tests with Verbose Output

```bash
./build/bin/test_basic_types --gtest_verbose
```

### Run Specific Test Cases

```bash
./build/bin/test_basic_types --gtest_filter="BasicTypesTest.DirAsInt"
```

## Test Organization

### Unit Tests (`unit/`)

Tests individual components in isolation:

- **test_basic_types.cpp**: Tests Dir, Coord, Polar types and Compass enum
- **test_neural_net_wiring.cpp**: Tests neural network wiring from genome
- **test_grid_visit_neighborhood.cpp**: Tests grid neighborhood visitation

### Integration Tests (`integration/`)

*Reserved for future use* - will test interaction between multiple components.

### Fixtures (`fixtures/`)

*Reserved for future use* - will contain test data, sample genomes, configuration files.

## Writing New Tests

### Create a New Unit Test

1. Create a new file in `tests/unit/`, e.g., `test_my_feature.cpp`
2. Include gtest and necessary headers:

```cpp
#include <gtest/gtest.h>
#include "my_feature.h"

using namespace BioSim;

class MyFeatureTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Setup code
  }

  void TearDown() override {
    // Cleanup code
  }
};

TEST_F(MyFeatureTest, TestSomething) {
  EXPECT_EQ(1 + 1, 2);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
```

3. Add the test to `tests/CMakeLists.txt`:

```cmake
add_biosim_test(test_my_feature unit/test_my_feature.cpp)
```

## Google Test Assertions

Common assertions:
- `EXPECT_EQ(a, b)` - Expects a == b
- `EXPECT_NE(a, b)` - Expects a != b
- `EXPECT_TRUE(condition)` - Expects condition is true
- `EXPECT_FALSE(condition)` - Expects condition is false
- `EXPECT_FLOAT_EQ(a, b)` - Expects floats are equal
- `EXPECT_NO_THROW(statement)` - Expects no exception
- `ASSERT_*` versions - Same as EXPECT but stop test on failure

## Benefits of Google Test

1. **Better test organization**: Test fixtures, setup/teardown
2. **Rich assertions**: Clear failure messages
3. **Test filtering**: Run specific tests easily
4. **Test discovery**: Automatic test registration
5. **Parameterized tests**: Run same test with different inputs
6. **Death tests**: Test code that should crash
7. **Integration with IDEs**: Better tooling support

## Clean Build

The `scripts/clean-build.sh` script preserves Google Test artifacts to avoid re-downloading:

```bash
./scripts/clean-build.sh
```

This keeps `_deps/googletest-*` directories intact.

## CI/CD Integration

Google Test is widely supported by CI/CD platforms:

```bash
# GitHub Actions, GitLab CI, etc.
ctest --output-on-failure --verbose
```

## Future Enhancements

- [ ] Add integration tests for full simulation cycles
- [ ] Add performance/benchmark tests
- [ ] Add test fixtures with sample data
- [ ] Set up code coverage reporting
- [ ] Add fuzzing tests for robustness
- [ ] Mock objects for isolated testing
- [ ] Parameterized tests for exhaustive coverage

## Migration Notes

Old unit tests (`unitTestBasicTypes.cpp`, etc.) have been converted to Google Test format and moved to this directory. The old `assert`-based tests have been replaced with gtest's `EXPECT_*` and `ASSERT_*` macros for better diagnostics.

Tests are now separated from the main application and can be run independently.
