cmake_minimum_required(VERSION 3.12)
project(python_ctools C)

set(CMAKE_C_STANDARD 99)
set(C_STANDARD_REQUIRED ON)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include_directories(src)
find_package(Python3 COMPONENTS Development)
if (NOT ${Python3_Development_FOUND})
    message(FATAL_ERROR "Can't find python lib" ${PYTHON3_INCLUDE_DIRS})
else ()
    message(STATUS "Include python dir: " ${PYTHON3_INCLUDE_DIRS})
endif ()

include_directories(${PYTHON3_INCLUDE_DIRS})

file(GLOB files LIST_DIRECTORIES false src/*.[ch])

Python3_add_library(ctools MODULE ${files})
