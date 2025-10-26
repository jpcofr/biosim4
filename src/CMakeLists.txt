
# Collect source files from new directory structure
file(GLOB_RECURSE BIOSIM_SOURCES
  types/*.cpp
  utils/*.cpp
  core/**/*.cpp
  io/**/*.cpp
)

# Only include main.cpp from root directory (all other files have been refactored into subdirectories)
set(ROOT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

# Exclude test files from source list (they end with _test.cpp)
file(GLOB_RECURSE TEST_SOURCES
  types/*_test.cpp
  utils/*_test.cpp
  core/**/*_test.cpp
  io/**/*_test.cpp
)
list(REMOVE_ITEM BIOSIM_SOURCES ${TEST_SOURCES})

# Combine all non-test sources
set(ALL_SOURCES ${ROOT_SOURCES} ${BIOSIM_SOURCES})

# Implementation-specific headers from src/ subdirectories
file(GLOB_RECURSE IMPL_HEADERS
  io/video/*.h
)

SET(GCC_COVERAGE_COMPILE_FLAGS "-O3 -Wall -fexceptions -fopenmp")
SET(GCC_COVERAGE_LINK_FLAGS    "-lpthread -O3 -lz -lgomp")

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")
set(CMAKE_VERBOSE_MAKEFILE on)

add_executable(biosim4 ${ALL_SOURCES} ${IMPL_HEADERS})

# Project includes (non-SYSTEM) - include both old and new paths during transition
target_include_directories(biosim4 PRIVATE
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

# Third-party headers as SYSTEM to mute their warnings
target_include_directories(biosim4 SYSTEM PRIVATE
  ${toml11_SOURCE_DIR}
  ${cli11_SOURCE_DIR}/include
  ${raylib_SOURCE_DIR}/src
  ${spdlog_SOURCE_DIR}/include
  ${FFMPEG_INCLUDE_DIRS}
)

# Mute deprecations only on this target (Clang/GCC)
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
  target_compile_options(biosim4 PRIVATE
    -Wno-deprecated-literal-operator     # toml11 UDL spacing (C++23)
    -Wno-deprecated-declarations         # CLI11 wstring_convert path
  )
endif()

# Link directories for FFmpeg
target_link_directories(biosim4 PRIVATE ${FFMPEG_LIBRARY_DIRS})

# Link libraries
# Note: spdlog includes fmt, no need for separate fmt::fmt
target_link_libraries(biosim4 PUBLIC
  raylib
  spdlog::spdlog
  ${OpenMP_CXX_LIBRARIES}
  toml11::toml11
  ${FFMPEG_LIBRARIES}
)

# Explicitly link libc++ for toml11 support (fixes __hash_memory symbol issue)
target_link_libraries(biosim4 PRIVATE
  /opt/homebrew/Cellar/llvm/21.1.2/lib/c++/libc++.1.0.dylib
)

# Enable video generation if requested
if(ENABLE_VIDEO_GENERATION)
    target_compile_definitions(biosim4 PRIVATE ENABLE_VIDEO_GENERATION)
    message(STATUS "Video generation: ENABLED")
endif()


install(TARGETS biosim4 DESTINATION bin)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
