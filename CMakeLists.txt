cmake_minimum_required(VERSION 3.2)
project(pyjion)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package (Python3 COMPONENTS Interpreter Development)
include_directories(${Python3_INCLUDE_DIRS})

set (CLR_DIR CoreCLR/src/coreclr)

add_compile_options(-fexceptions)
add_definitions(-DUSE_STL)
add_compile_options(-fvisibility=hidden)

if (DEBUG)
    add_definitions(-DDEBUG_CALL_TRACE)
endif()

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

if (WIN32)
    set(CLR_OS_BUILD WIN.x64.Debug)
    set(CLR_JIT_LIB "clrjit.dll")
endif()

if (LINUX)
    set(CLR_OS_BUILD OSX.x64.Debug)
    set(CLR_JIT_LIB "libclrjit.so")
endif()

if (APPLE)
    set(CLR_OS_BUILD OSX.x64.Debug)
    include_directories(Pyjion/compat)
    set(CLR_CMAKE_HOST_UNIX 1)
    set(CLR_JIT_LIB "libclrjit.dylib")
endif()

include_directories(CoreCLR/artifacts/bin/coreclr/${CLR_OS_BUILD}/inc)
set(SOURCES Pyjion/absint.cpp Pyjion/absvalue.cpp Pyjion/intrins.cpp Pyjion/jitinit.cpp Pyjion/pycomp.cpp Pyjion/pyjit.cpp)

add_library(pyjion MODULE ${SOURCES})

install(TARGETS pyjion
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

set_target_properties(
        pyjion
        PROPERTIES
        PREFIX ""
        OUTPUT_NAME "pyjion"
        LINKER_LANGUAGE C
)

target_link_libraries(pyjion ${Python3_LIBRARIES})
target_link_libraries(pyjion ${CMAKE_SOURCE_DIR}/CoreCLR/artifacts/bin/coreclr/${CLR_OS_BUILD}/${CLR_JIT_LIB})
