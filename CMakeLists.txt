cmake_minimum_required(VERSION 4.1.2)
project(BioSim4 VERSION 0.2.0)

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
option(USE_FETCHCONTENT_OPENCV "Build OpenCV from source (slow first time, ~30-60min)" OFF)
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

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Make Google Test available
FetchContent_MakeAvailable(googletest)

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

# ============================================================================
# OpenCV Configuration - Choose your approach
# ============================================================================

if(USE_FETCHCONTENT_OPENCV)
    # OPTION A: Build from source (self-contained, ~30-60 min first time)
    message(STATUS "Building OpenCV from source via FetchContent...")
    message(STATUS "  This will take 30-60 minutes on first build")
    message(STATUS "  Subsequent builds will be fast (cached)")

    # Configure minimal OpenCV build
    set(BUILD_LIST "core,imgproc,highgui,videoio,video,imgcodecs" CACHE STRING "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_PERF_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_apps OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_python2 OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_python3 OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_java OFF CACHE BOOL "" FORCE)

    # Enable video support
    set(WITH_FFMPEG ON CACHE BOOL "" FORCE)
    set(WITH_AVFOUNDATION ON CACHE BOOL "" FORCE)
    set(WITH_OPENMP ON CACHE BOOL "" FORCE)
    set(WITH_PNG ON CACHE BOOL "" FORCE)
    set(WITH_JPEG ON CACHE BOOL "" FORCE)

    # Disable GUI
    set(WITH_QT OFF CACHE BOOL "" FORCE)
    set(WITH_GTK OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        opencv
        GIT_REPOSITORY https://github.com/opencv/opencv.git
        GIT_TAG 4.8.1
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )

    FetchContent_MakeAvailable(opencv)

    set(OpenCV_LIBS opencv_core opencv_imgproc opencv_highgui opencv_videoio opencv_video opencv_imgcodecs)
    include_directories(${opencv_SOURCE_DIR}/include ${opencv_BINARY_DIR})

else()
    # OPTION B: Use Homebrew OpenCV (RECOMMENDED - fast setup)
    message(STATUS "Using Homebrew OpenCV...")
    message(STATUS "  If not installed: brew install opencv")

    # Force use of Homebrew OpenCV by setting explicit paths
    # This prevents CMake from finding incomplete custom builds
    set(OpenCV_DIR "/opt/homebrew/lib/cmake/opencv4" CACHE PATH "Path to OpenCV config" FORCE)

    # Use Homebrew's OpenCV
    find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui videoio video imgcodecs)

    if(OpenCV_FOUND)
        message(STATUS "  ✓ Found OpenCV ${OpenCV_VERSION}")
        message(STATUS "  ✓ Include: ${OpenCV_INCLUDE_DIRS}")
        message(STATUS "  ✓ Libraries: ${OpenCV_LIBS}")
    else()
        message(FATAL_ERROR "OpenCV not found! Install with: brew install opencv")
    endif()
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
