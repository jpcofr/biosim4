
FILE(GLOB MyCppSources ./*.cpp)
# Only include implementation-specific headers (CImg.h, imageWriter.h) from src/
# All other headers are now in ../include/
FILE(GLOB MyCppheaders ./CImg.h ./imageWriter.h)

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

# Include directories: public headers from ../include/ and OpenCV
target_include_directories(biosim4 PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${OpenCV_INCLUDE_DIRS}
)

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
