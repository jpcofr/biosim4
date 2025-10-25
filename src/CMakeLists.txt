
FILE(GLOB MyCppSources ./*.cpp)
FILE(GLOB MyCppheaders ../include/*.h)

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

SET(GCC_COVERAGE_COMPILE_FLAGS "-O3 -Wall -fexceptions -fopenmp")
SET(GCC_COVERAGE_LINK_FLAGS    "-lpthread -O3 -lz -lgomp")

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")
set(CMAKE_VERBOSE_MAKEFILE on)

add_executable(biosim4 ${MyCppSources} ${MyCppheaders})

# Include OpenCV headers
target_include_directories(biosim4 PRIVATE ${OpenCV_INCLUDE_DIRS})
target_include_directories(biosim4 PUBLIC ${PROJECT_SOURCE_DIR}/include)

# Link libraries
target_link_libraries(biosim4 PUBLIC ${OpenCV_LIBS})

# Enable video generation if requested
if(ENABLE_VIDEO_GENERATION)
    target_compile_definitions(biosim4 PRIVATE ENABLE_VIDEO_GENERATION)
    message(STATUS "Video generation: ENABLED")
endif()


install(TARGETS biosim4 DESTINATION bin)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)


