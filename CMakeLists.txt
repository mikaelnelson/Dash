# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include(ExternalProject)

include(/Users/mikaelnelson/esp/esp-idf/tools/cmake/project.cmake)
project(Dash C CXX ASM)

#set(CMAKE_DISABLE_SOURCE_CHANGES OFF)

# libj1939
ExternalProject_Add(
        libj1939
        GIT_REPOSITORY https://github.com/paoloteti/libj1939.git
        GIT_TAG f98b7c9efea799333d33df55678ae6338da018b0
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/source/libj1939
        BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/build/libj1939
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/external/install/libj1939
        INSTALL_COMMAND mkdir -p ${CMAKE_CURRENT_SOURCE_DIR}/external/install/libj1939/include/ COMMAND mkdir -p ${CMAKE_CURRENT_SOURCE_DIR}/external/install/libj1939/lib/ COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/external/source/libj1939/include/j1939.h ${CMAKE_CURRENT_SOURCE_DIR}/external/install/libj1939/include/j1939.h COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/external/build/libj1939/libj1939.a ${CMAKE_CURRENT_SOURCE_DIR}/external/install/libj1939/lib/libj1939.a
)
