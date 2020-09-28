cmake_minimum_required(VERSION 3.2)
project(pyjion)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CheckCXXCompilerFlag)

find_package (Python3 COMPONENTS Interpreter Development)
include_directories(${Python3_INCLUDE_DIRS})

set (CLR_DIR CoreCLR/src/coreclr)

add_compile_options(-fexceptions)
add_definitions(-DUSE_STL)

if(NOT WIN32)
    include_directories(CoreCLR/src/coreclr/src/pal/inc/rt CoreCLR/src/coreclr/src/pal/inc CoreCLR/src/coreclr/src/pal/prebuilt/inc)
    add_compile_options(-DPAL_STDCPP_COMPAT)
    add_compile_options(-fdeclspec)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-Wno-null-arithmetic)
    else(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-Wno-conversion-null -Wno-pointer-arith)
    endif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
endif(NOT WIN32)

include_directories(CoreCLR/src/coreclr/src/inc)

if (APPLE)
    include_directories(Pyjion/compat)
    include_directories(CoreCLR/artifacts/bin/coreclr/OSX.x64.Debug/inc)
    set(CLR_CMAKE_HOST_UNIX 1)
endif()

set(SOURCES Pyjion/absint.cpp Pyjion/absvalue.cpp Pyjion/intrins.cpp Pyjion/jitinit.cpp Pyjion/pycomp.cpp)

add_library(pyjion SHARED ${SOURCES})
target_link_libraries(pyjion ${Python3_LIBRARIES})
target_link_libraries(pyjion ${CMAKE_SOURCE_DIR}/CoreCLR/artifacts/bin/coreclr/OSX.x64.Debug/libclrjit.dylib)
add_compile_options(-Wno-sign-compare)
