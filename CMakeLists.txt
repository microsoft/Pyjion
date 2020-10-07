cmake_minimum_required(VERSION 3.2)
project(pyjion)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package (Python3 3.9 COMPONENTS Interpreter Development)
include_directories(${Python3_INCLUDE_DIRS})

set (CLR_DIR CoreCLR/src/coreclr)

add_definitions(-DUSE_STL)

IF(CMAKE_BUILD_TYPE MATCHES Debug)
    message(STATUS "Enabling very verbose messages")
    add_definitions(-DDEBUG)
ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)

if(NOT WIN32)
    set(CMAKE_CXX_STANDARD 14)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    include_directories(CoreCLR/src/coreclr/src/pal/inc/rt CoreCLR/src/coreclr/src/pal/inc CoreCLR/src/coreclr/src/pal/prebuilt/inc)
    add_compile_options(-DPAL_STDCPP_COMPAT)
    add_compile_options(-fexceptions)
    add_compile_options(-fvisibility=hidden)

    add_definitions(-DTARGET_UNIX)
    message(STATUS "Enabling UNIX Patches")
    add_compile_options(-Wswitch)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-Wno-null-arithmetic)
    else(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-Wno-conversion-null -Wno-pointer-arith)
    endif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
else()
    add_definitions(-DWINDOWS)
    if (MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest")
    endif(MSVC)
endif(NOT WIN32)


add_definitions(-DPROFILING_SUPPORTED -DUNICODE)

if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
    set(IS_64 1)
elseif (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "amd64")
    set(IS_64 1)
elseif (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "AMD64")
    if (CMAKE_CL_64)
        set(IS_64 1)
    endif()
endif()

if (IS_64)
    add_definitions(-D_TARGET_AMD64_)
    add_definitions(-DTARGET_AMD64)
    add_definitions(-DTARGET_64BIT)
    add_definitions(-DHOST_64BIT)
    add_definitions(-DHOST_AMD64)
    message(STATUS "Enabling AMD64")
    if (NOT WIN32)
        add_definitions(-DUNIX_AMD64_ABI)
        add_definitions(-DUNIX_AMD64_ABI_ITF)
        message(STATUS "Enabling AMD64 ABI")
    endif()
else()
    add_definitions(-DTARGET_X86)
    message(STATUS "Enabling x86")
endif()

include_directories(CoreCLR/src/coreclr/src/inc CoreCLR/src/coreclr/src/jit)

if (WIN32)
    set(CLR_OS_BUILD Windows_NT.x64.Debug)
    set(CLR_JIT_LIB "lib/coreclr_static.lib")
endif()

if (LINUX)
    set(CLR_OS_BUILD Linux.x64.Debug)
    set(CLR_JIT_LIB "libclrjit.so")
endif()

if (APPLE)
    set(CLR_OS_BUILD OSX.x64.Debug)
    include_directories(Pyjion/compat)
    set(CLR_CMAKE_HOST_UNIX 1)
    set(CLR_JIT_LIB "libclrjit.dylib")
    add_definitions(-D_XOPEN_SOURCE)
    add_definitions(-DTARGET_OSX)
    add_compile_options(-fdeclspec)
    add_definitions(-DFEATURE_SIMD)
    add_definitions(-DFEATURE_HW_INTRINSICS)
endif()

include_directories(CoreCLR/artifacts/bin/coreclr/${CLR_OS_BUILD}/inc)
set(SOURCES Pyjion/absint.cpp Pyjion/absvalue.cpp Pyjion/intrins.cpp Pyjion/jitinit.cpp Pyjion/pycomp.cpp Pyjion/pyjit.cpp Pyjion/disasm.cpp)

add_library(pyjionlib OBJECT ${SOURCES})
add_library(pyjion MODULE $<TARGET_OBJECTS:pyjionlib>)
install(TARGETS pyjion
        LIBRARY DESTINATION ${CMAKE_BINARY_DIR})

set_target_properties(
        pyjion
        PROPERTIES
        PREFIX ""
        OUTPUT_NAME "pyjion"
        LINKER_LANGUAGE C
)

target_link_libraries(pyjion ${Python3_LIBRARIES})
target_link_libraries(pyjion ${CMAKE_SOURCE_DIR}/CoreCLR/artifacts/bin/coreclr/${CLR_OS_BUILD}/${CLR_JIT_LIB})

# Testing
add_subdirectory(Tests/Catch)
set(TEST_SOURCES Tests/testing_util.cpp Tests/test_emission.cpp Tests/test_inference.cpp Tests/test_compiler.cpp Tests/test_absint.cpp Tests/Tests.cpp)
add_executable(unit_tests ${TEST_SOURCES} $<TARGET_OBJECTS:pyjionlib>)
target_compile_definitions(unit_tests PUBLIC -DDUMP_TRACES)
target_include_directories(unit_tests PRIVATE Pyjion)
target_link_libraries(unit_tests Catch2::Catch2)
target_link_libraries(unit_tests ${Python3_LIBRARIES})
target_link_libraries(unit_tests ${CMAKE_SOURCE_DIR}/CoreCLR/artifacts/bin/coreclr/${CLR_OS_BUILD}/${CLR_JIT_LIB})
