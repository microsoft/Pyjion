cmake_minimum_required(VERSION 3.2)
project(pyjion)

set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED)


find_package (Python3 COMPONENTS Interpreter Development)
include_directories(${Python3_INCLUDE_DIRS})

set(SOURCES Pyjion/absint.cpp Pyjion/absvalue.cpp Pyjion/intrins.cpp Pyjion/jitinit.cpp Pyjion/pycomp.cpp)
add_library(pyjion SHARED ${SOURCES})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-sign-compare -Wno-invalid-token-paste")

option(WERROR "Turn warnings into errors" OFF)
if (WERROR)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
endif ()