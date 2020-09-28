cmake_minimum_required(VERSION 3.2)
project(pyjion)

set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED)


find_package (Python3 COMPONENTS Interpreter Development)
include_directories(${Python3_INCLUDE_DIRS})

#set (CLR_DIR CoreCLR/src/coreclr)
#
#include(CoreCLR/eng/native/configurecompiler.cmake)
#include(CoreCLR/eng/native/configureplatform.cmake)
#include(CoreCLR/eng/native/functions.cmake)
#include(CoreCLR/src/coreclr/pgosupport.cmake)
#set(CLR_CMAKE_TARGET_ARCH_AMD64 1)

#add_subdirectory(CoreCLR/src/coreclr/src/jit)
#add_subdirectory(CoreCLR/src/coreclr/src/pal)
include_directories(CoreCLR/src/coreclr/src/pal/inc)
include_directories(CoreCLR/src/coreclr/src/pal/inc/rt)
include_directories(CoreCLR/src/coreclr/src/pal/prebuilt/inc)
include_directories(CoreCLR/src/coreclr/src/inc)

set(SOURCES Pyjion/absint.cpp Pyjion/absvalue.cpp Pyjion/intrins.cpp Pyjion/jitinit.cpp Pyjion/pycomp.cpp)
add_library(pyjion SHARED ${SOURCES})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-sign-compare")

option(WERROR "Turn warnings into errors" OFF)
if (WERROR)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
endif ()