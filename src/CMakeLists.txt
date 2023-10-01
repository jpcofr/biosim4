
FILE(GLOB MyCppSources ./*.cpp)
FILE(GLOB MyCppheaders ../include/*.h)

SET(GCC_COVERAGE_COMPILE_FLAGS "-O3 -Wall -fexceptions -fopenmp")
SET(GCC_COVERAGE_LINK_FLAGS    "-lpthread -O3 -lz -lgomp")

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")

add_executable(biosim4 ${MyCppSources} ${MyCppheaders})
target_link_libraries( biosim4 PUBLIC ${OpenCV_LIBS} )
target_include_directories(biosim4 PUBLIC ${PROJECT_SOURCE_DIR}/include)


install(TARGETS biosim4 DESTINATION bin)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)


