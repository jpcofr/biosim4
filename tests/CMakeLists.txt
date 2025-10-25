# tests/CMakeLists.txt
# Test suite using Google Test

enable_testing()

# Gather all source files from src/ that are needed for testing
# Exclude main.cpp and the old unit test files
file(GLOB BIOSIM_LIB_SOURCES
  ${CMAKE_SOURCE_DIR}/src/*.cpp
)

# Remove main.cpp and old unit test files from library sources
list(FILTER BIOSIM_LIB_SOURCES EXCLUDE REGEX ".*main\\.cpp$")
list(FILTER BIOSIM_LIB_SOURCES EXCLUDE REGEX ".*unitTest.*\\.cpp$")

# Create a library from biosim4 sources (excluding main)
# This allows tests to link against the core functionality
add_library(biosim4_lib STATIC ${BIOSIM_LIB_SOURCES})

# Include directories for the library
target_include_directories(biosim4_lib PUBLIC
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/src
  ${OpenCV_INCLUDE_DIRS}
  ${toml11_SOURCE_DIR}
  ${cli11_SOURCE_DIR}/include
)

# Link OpenCV to the library
target_link_libraries(biosim4_lib PUBLIC ${OpenCV_LIBS})

# Apply the same compilation and linking flags as the main executable
target_compile_options(biosim4_lib PRIVATE -O3 -Wall -fexceptions -fopenmp)
target_link_libraries(biosim4_lib PUBLIC pthread z omp)

# Enable video generation if requested
if(ENABLE_VIDEO_GENERATION)
  target_compile_definitions(biosim4_lib PRIVATE ENABLE_VIDEO_GENERATION)
endif()

# Function to create a test executable
function(add_biosim_test TEST_NAME TEST_SOURCE)
  add_executable(${TEST_NAME} ${TEST_SOURCE})

  target_link_libraries(${TEST_NAME}
    PRIVATE
      biosim4_lib
      GTest::gtest
      GTest::gtest_main
  )

  target_include_directories(${TEST_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
  )

  # Add test to CTest (even though we're using gtest, this allows "ctest" to work)
  add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})

  # Set test properties
  set_tests_properties(${TEST_NAME} PROPERTIES
    TIMEOUT 30
    LABELS "unit"
  )
endfunction()

# Add unit tests
add_biosim_test(test_basic_types unit/test_basic_types.cpp)
add_biosim_test(test_neural_net_wiring unit/test_neural_net_wiring.cpp)
add_biosim_test(test_grid_visit_neighborhood unit/test_grid_visit_neighborhood.cpp)

# Optional: Add a target to run all tests
add_custom_target(run_tests
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
  DEPENDS test_basic_types test_neural_net_wiring test_grid_visit_neighborhood
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMENT "Running all tests..."
)

message(STATUS "Tests configured with Google Test")
message(STATUS "  - Unit tests: test_basic_types, test_neural_net_wiring, test_grid_visit_neighborhood")
message(STATUS "  - Run tests with: make test or ninja test")
message(STATUS "  - Or run individual tests from: ${CMAKE_BINARY_DIR}/bin/")
