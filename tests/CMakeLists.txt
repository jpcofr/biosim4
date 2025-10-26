# tests/CMakeLists.txt
# Test suite using Google Test

enable_testing()

# Gather all source files from src/ that are needed for testing
# Exclude main.cpp and the old unit test files
file(GLOB_RECURSE BIOSIM_LIB_SOURCES
  ${CMAKE_SOURCE_DIR}/src/*.cpp
  ${CMAKE_SOURCE_DIR}/src/types/*.cpp
  ${CMAKE_SOURCE_DIR}/src/utils/*.cpp
  ${CMAKE_SOURCE_DIR}/src/core/**/*.cpp
  ${CMAKE_SOURCE_DIR}/src/io/**/*.cpp
)

# Remove main.cpp, old unit test files, and new *_test.cpp files from library sources
list(FILTER BIOSIM_LIB_SOURCES EXCLUDE REGEX ".*main\\.cpp$")
list(FILTER BIOSIM_LIB_SOURCES EXCLUDE REGEX ".*unitTest.*\\.cpp$")
list(FILTER BIOSIM_LIB_SOURCES EXCLUDE REGEX ".*_test\\.cpp$")

# Create a library from biosim4 sources (excluding main)
# This allows tests to link against the core functionality
add_library(biosim4_lib STATIC ${BIOSIM_LIB_SOURCES})

# Include directories for the library - support both old and new structure
target_include_directories(biosim4_lib PUBLIC
  ${CMAKE_SOURCE_DIR}/include          # Old headers (backward compatibility)
  ${CMAKE_SOURCE_DIR}/src              # Old sources (backward compatibility)
  ${CMAKE_SOURCE_DIR}/src/types        # New structure
  ${CMAKE_SOURCE_DIR}/src/utils
  ${CMAKE_SOURCE_DIR}/src/core
  ${CMAKE_SOURCE_DIR}/src/core/world
  ${CMAKE_SOURCE_DIR}/src/core/agents
  ${CMAKE_SOURCE_DIR}/src/core/genetics
  ${CMAKE_SOURCE_DIR}/src/core/simulation
  ${CMAKE_SOURCE_DIR}/src/io
  ${CMAKE_SOURCE_DIR}/src/io/config
  ${CMAKE_SOURCE_DIR}/src/io/video
  ${CMAKE_SOURCE_DIR}/src/io/render
  ${cli11_SOURCE_DIR}/include
)

# Suppress warnings coming from toml11, raylib, and FFmpeg
target_include_directories(biosim4_lib SYSTEM PUBLIC
  ${toml11_SOURCE_DIR}
  ${raylib_SOURCE_DIR}/src
  ${spdlog_SOURCE_DIR}/include
  ${FFMPEG_INCLUDE_DIRS}
)

# Link directories for FFmpeg
target_link_directories(biosim4_lib PUBLIC ${FFMPEG_LIBRARY_DIRS})

# Link raylib, spdlog, and FFmpeg to the library
target_link_libraries(biosim4_lib PUBLIC raylib spdlog::spdlog ${FFMPEG_LIBRARIES})

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
    ${CMAKE_SOURCE_DIR}/include          # Old headers (backward compatibility)
    ${CMAKE_SOURCE_DIR}/src              # Old sources (backward compatibility)
    ${CMAKE_SOURCE_DIR}/src/types        # New structure
    ${CMAKE_SOURCE_DIR}/src/utils
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/core/world
    ${CMAKE_SOURCE_DIR}/src/core/agents
    ${CMAKE_SOURCE_DIR}/src/core/genetics
    ${CMAKE_SOURCE_DIR}/src/core/simulation
    ${CMAKE_SOURCE_DIR}/src/io
    ${CMAKE_SOURCE_DIR}/src/io/config
    ${CMAKE_SOURCE_DIR}/src/io/video
    ${CMAKE_SOURCE_DIR}/src/io/render
  )

  # Add test to CTest (even though we're using gtest, this allows "ctest" to work)
  add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})

  # Set test properties
  set_tests_properties(${TEST_NAME} PROPERTIES
    TIMEOUT 30
    LABELS "unit"
  )
endfunction()

# Legacy unit tests removed - all tests are now co-located with source code

# Auto-discover and add co-located tests from src/ subdirectories
file(GLOB_RECURSE COLOCATED_TEST_SOURCES
  ${CMAKE_SOURCE_DIR}/src/types/*_test.cpp
  ${CMAKE_SOURCE_DIR}/src/utils/*_test.cpp
  ${CMAKE_SOURCE_DIR}/src/core/**/*_test.cpp
  ${CMAKE_SOURCE_DIR}/src/io/**/*_test.cpp
)

foreach(TEST_SOURCE ${COLOCATED_TEST_SOURCES})
  get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
  add_biosim_test(${TEST_NAME} ${TEST_SOURCE})
endforeach()

# Collect all test targets for the run_tests target
set(ALL_TEST_TARGETS)
foreach(TEST_SOURCE ${COLOCATED_TEST_SOURCES})
  get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
  list(APPEND ALL_TEST_TARGETS ${TEST_NAME})
endforeach()

# Optional: Add a target to run all tests
add_custom_target(run_tests
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
  DEPENDS ${ALL_TEST_TARGETS}
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMENT "Running all tests..."
)

message(STATUS "Tests configured with Google Test")
message(STATUS "  - Co-located tests: ${COLOCATED_TEST_SOURCES}")
message(STATUS "  - Run tests with: make test or ninja test")
message(STATUS "  - Or run individual tests from: ${CMAKE_BINARY_DIR}/bin/")
