cmake_minimum_required(VERSION 4.1.2)
project(BioSim4 VERSION 0.2.0)

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set default parallel build level if not already set
if(NOT CMAKE_BUILD_PARALLEL_LEVEL)
    include(ProcessorCount)
    ProcessorCount(N)
    set(CMAKE_BUILD_PARALLEL_LEVEL ${N})
endif()

# Ensure all build outputs go into ./build/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Build options
option(ENABLE_VIDEO_GENERATION "Enable video file generation" ON)
option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer for memory leak detection" OFF)
option(ENABLE_THREAD_SANITIZER "Enable ThreadSanitizer for race condition detection (cannot be used with ENABLE_SANITIZERS)" OFF)
option(BUILD_DOCUMENTATION "Build Doxygen documentation" OFF)

include(FetchContent)

# Fetch Google Test from GitHub
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.17.0
  GIT_SHALLOW TRUE
)

# Fetch toml11 for modern TOML configuration parsing
FetchContent_Declare(
  toml11
  GIT_REPOSITORY https://github.com/ToruNiina/toml11.git
  GIT_TAG v3.8.1
  GIT_SHALLOW TRUE
)

# Fetch CLI11 for command-line parsing
FetchContent_Declare(
  cli11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
  GIT_TAG v2.4.1
  GIT_SHALLOW TRUE
)

# Fetch raylib for rendering (replaces OpenCV/CImg)
FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.5
  GIT_SHALLOW TRUE
)

# Configure raylib build options (disable unused features)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(SUPPORT_MODULE_RAUDIO OFF CACHE BOOL "" FORCE)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Make dependencies available
FetchContent_MakeAvailable(googletest toml11 cli11 raylib)

# Find FFmpeg libraries using pkg-config (for static linking)
find_package(PkgConfig REQUIRED)

# Try to find FFmpeg via pkg-config with preference for static libraries
pkg_check_modules(FFMPEG REQUIRED
  libavcodec
  libavformat
  libavutil
  libswscale
)

# Prefer static libraries if available
if(FFMPEG_STATIC_FOUND)
  set(FFMPEG_LIBRARIES ${FFMPEG_STATIC_LIBRARIES})
  set(FFMPEG_INCLUDE_DIRS ${FFMPEG_STATIC_INCLUDE_DIRS})
  set(FFMPEG_LIBRARY_DIRS ${FFMPEG_STATIC_LIBRARY_DIRS})
  message(STATUS "Using static FFmpeg libraries")
else()
  set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES})
  set(FFMPEG_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIRS})
  set(FFMPEG_LIBRARY_DIRS ${FFMPEG_LIBRARY_DIRS})
  message(STATUS "Using dynamic FFmpeg libraries")
endif()

message(STATUS "FFmpeg include dirs: ${FFMPEG_INCLUDE_DIRS}")
message(STATUS "FFmpeg library dirs: ${FFMPEG_LIBRARY_DIRS}")
message(STATUS "FFmpeg libraries: ${FFMPEG_LIBRARIES}")

set(CMAKE_C_COMPILER "/opt/homebrew/opt/llvm/bin/clang")
set(CMAKE_CXX_COMPILER "/opt/homebrew/opt/llvm/bin/clang++")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CPPFLAGS} ${LDFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CPPFLAGS} ${LDFLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LDFLAGS}")

# Sanitizer configuration
if(ENABLE_SANITIZERS AND ENABLE_THREAD_SANITIZER)
    message(FATAL_ERROR "Cannot enable both ENABLE_SANITIZERS and ENABLE_THREAD_SANITIZER simultaneously")
endif()

if(ENABLE_SANITIZERS)
    message(STATUS "Building with AddressSanitizer and UndefinedBehaviorSanitizer")
    set(SANITIZER_FLAGS "-fsanitize=address,undefined -fno-omit-frame-pointer -g")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZER_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_FLAGS}")
endif()

if(ENABLE_THREAD_SANITIZER)
    message(STATUS "Building with ThreadSanitizer")
    set(SANITIZER_FLAGS "-fsanitize=thread -fno-omit-frame-pointer -g")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZER_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_FLAGS}")
endif()

# Enable testing for the whole project
enable_testing()

add_subdirectory(src)
add_subdirectory(tests)

# ============================================================================
# Doxygen Documentation
# ============================================================================
if(BUILD_DOCUMENTATION)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        message(STATUS "Doxygen found: ${DOXYGEN_EXECUTABLE}")

        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

        # Copy Doxyfile to build directory
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} COPYONLY)

        # Add custom target to build documentation
        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )

        message(STATUS "Documentation target 'docs' enabled")
        message(STATUS "  Run: ninja docs")
        message(STATUS "  Output: ${CMAKE_CURRENT_BINARY_DIR}/docs/html/index.html")
    else()
        message(WARNING "Doxygen not found. Install with: brew install doxygen graphviz")
        message(WARNING "Documentation target will not be available")
    endif()
endif()

install(FILES ./config/biosim4.ini DESTINATION config)
install(DIRECTORY ./tools/ DESTINATION tools)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
