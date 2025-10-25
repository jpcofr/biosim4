cmake_minimum_required(VERSION 3.25)
project(BioSim4 VERSION 0.2.0)

# Build options
option(USE_FETCHCONTENT_OPENCV "Build OpenCV from source (slow first time, ~30-60min)" OFF)
option(ENABLE_VIDEO_GENERATION "Enable video file generation" ON)
option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer for memory leak detection" OFF)
option(ENABLE_THREAD_SANITIZER "Enable ThreadSanitizer for race condition detection (cannot be used with ENABLE_SANITIZERS)" OFF)

include(FetchContent)
include(CTest)

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

enable_testing()

add_subdirectory(src)
add_subdirectory(include)

include_directories(${PROJECT_SOURCE_DIR}/include)

install(FILES ./config/biosim4.ini DESTINATION config)
install(DIRECTORY ./tools/ DESTINATION tools)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
