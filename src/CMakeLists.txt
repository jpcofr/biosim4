
FILE(GLOB MyCppSources ./*.cpp)
# Only include implementation-specific headers from src/
# All other headers are now in ../include/
FILE(GLOB MyCppheaders ./imageWriter.h ./videoVerifier.h)

# Exclude old unit test files - now handled by tests/ directory
list(REMOVE_ITEM MyCppSources
  "${CMAKE_CURRENT_SOURCE_DIR}/unitTestBasicTypes.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/unitTestConnectNeuralNetWiringFromGenome.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/unitTestGridVisitNeighborhood.cpp"
)

SET(GCC_COVERAGE_COMPILE_FLAGS "-O3 -Wall -fexceptions -fopenmp")
SET(GCC_COVERAGE_LINK_FLAGS    "-lpthread -O3 -lz -lgomp")

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")
set(CMAKE_VERBOSE_MAKEFILE on)

add_executable(biosim4 ${MyCppSources} ${MyCppheaders})

# Project includes (non-SYSTEM)
target_include_directories(biosim4 PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/src
)

# Third-party headers as SYSTEM to mute their warnings
target_include_directories(biosim4 SYSTEM PRIVATE
  ${toml11_SOURCE_DIR}
  ${cli11_SOURCE_DIR}/include
  ${raylib_SOURCE_DIR}/src
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
target_link_libraries(biosim4 PUBLIC
  raylib
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
