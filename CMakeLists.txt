cmake_minimum_required(VERSION 3.13)
project(ctools C)

set(CMAKE_C_STANDARD 99)

include_directories(.)

add_executable(ctools
        src/ctools_utils.c
        src/ctools_lfu.c
        src/ctools_config.h)

find_package(PythonLibs REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})
target_link_libraries(ctools ${PYTHON_LIBRARIES})