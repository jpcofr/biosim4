cmake_minimum_required(VERSION 3.25)
project(BioSim4 VERSION 0.2.0)

include(CTest)

set(CMAKE_C_COMPILER "/opt/homebrew/opt/llvm/bin/clang")
set(CMAKE_CXX_COMPILER "/opt/homebrew/opt/llvm/bin/clang++")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CPPFLAGS} ${LDFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CPPFLAGS} ${LDFLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LDFLAGS}")

enable_testing()

add_subdirectory(src)
add_subdirectory(include)

include_directories(${PROJECT_SOURCE_DIR}/include)

install(FILES ./biosim4.ini DESTINATION ./)
install(DIRECTORY ./tools/ DESTINATION tools)
install (DIRECTORY DESTINATION "logs" DIRECTORY_PERMISSIONS
    OWNER_WRITE OWNER_READ OWNER_EXECUTE
    GROUP_READ GROUP_READ GROUP_EXECUTE
    GROUP_READ WORLD_READ WORLD_EXECUTE)
install (DIRECTORY DESTINATION "images" DIRECTORY_PERMISSIONS
    OWNER_WRITE OWNER_READ OWNER_EXECUTE
    GROUP_READ GROUP_READ GROUP_EXECUTE
    GROUP_READ WORLD_READ WORLD_EXECUTE)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
