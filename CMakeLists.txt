cmake_minimum_required(VERSION 3.13)
project(ctools C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(ctools
        ctools.h
        ctoolsmodule.c
        lfu.h)

find_package(PythonLibs REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})
target_link_libraries(ctools ${PYTHON_LIBRARIES})


