#!/bin/bash

# Build things which don't depend upon CoreCLR
OUT_DIR=Pyjion
SRC_DIR=Pyjion
PY_INC_DIRS="-I../Python/Include/ -IPython/"
clang++-3.5 -DPLATFORM_UNIX=1 -std=c++11 $SRC_DIR/intrins.cpp  $PY_INC_DIRS -o $OUT_DIR/intrins.o -c -fPIC -g  -D_TARGET_AMD64_=1 
clang++-3.5 -DPLATFORM_UNIX=1 -std=c++11 $SRC_DIR/absvalue.cpp $PY_INC_DIRS  -o $OUT_DIR/absvalue.o -c -fPIC -g -D_TARGET_AMD64_=1 
clang++-3.5 -DPLATFORM_UNIX=1 -std=c++11 $SRC_DIR/absint.cpp $PY_INC_DIRS  -o $OUT_DIR/absint.o -c -fPIC -g  -D_TARGET_AMD64_=1 
clang++-3.5 -DPLATFORM_UNIX=1 -std=c++11 $SRC_DIR/jitinit.cpp $PY_INC_DIRS  -o $OUT_DIR/jitinit.o -c -fPIC -g -D_TARGET_AMD64_=1 
clang++-3.5 -DPLATFORM_UNIX=1 -std=c++11 $SRC_DIR/pyjit.cpp $PY_INC_DIRS  -o $OUT_DIR/pyjit.o -c -fPIC -g -D_TARGET_AMD64_=1 
clang++-3.5 -DPLATFORM_UNIX=1 -std=c++11 $SRC_DIR/bridge.cpp -o $OUT_DIR/bridge.o -c -fPIC -g -D_TARGET_AMD64_=1 
clang++-3.5 -DPLATFORM_UNIX=1 -std=c++11 $SRC_DIR/ipycomp.cpp -o $OUT_DIR/ipycomp.o -c -fPIC -g -D_TARGET_AMD64_=1 

# Build the CoreCLR integration
CORECLR_INCS="-ICoreCLR/src/pal/prebuilt/inc -ICoreCLR/src/pal/inc -ICoreCLR/src/pal/inc/rt  -ICoreCLR/src/inc   -ICoreCLR/src/pal/inc/rt/cpp"
clang++-3.5 -DFEATURE_PAL_SXS   -DAMD64 -DBIT64=1 -DFEATURE_CORECLR -DFEATURE_PAL -DFEATURE_PAL_ANSI  -DLINUX64 -DPLATFORM_UNIX=1 -DUNICODE -DUNIX_AMD64_ABI -D_AMD64_ -D_TARGET_AMD64_=1 -D_UNICODE -D_WIN64 $CORECLR_INCS -Wall -std=c++11 -g  -fno-omit-frame-pointer -fms-extensions -fstack-protector-strong -Werror -Wno-microsoft -nostdinc -o $OUT_DIR/cee.o    -c $SRC_DIR/cee.cpp    -c -fPIC -Wtautological-undefined-compare -Wno-invalid-noreturn -Wno-tautological-compare -Wno-invalid-noreturn
clang++-3.5 -DFEATURE_PAL_SXS   -DAMD64 -DBIT64=1 -DFEATURE_CORECLR -DFEATURE_PAL -DFEATURE_PAL_ANSI  -DLINUX64 -DPLATFORM_UNIX=1 -DUNICODE -DUNIX_AMD64_ABI -D_AMD64_ -D_TARGET_AMD64_=1 -D_UNICODE -D_WIN64 $CORECLR_INCS -Wall -std=c++11 -g  -fno-omit-frame-pointer -fms-extensions -fstack-protector-strong -Werror -Wno-microsoft -nostdinc -o $OUT_DIR/pycomp.o -c $SRC_DIR/pycomp.cpp -c -fPIC -Wno-invalid-noreturn

# And link it all together...
clang++-3.5 -shared -o   $OUT_DIR/pyjion.so $OUT_DIR/absint.o  $OUT_DIR/absvalue.o  $OUT_DIR/intrins.o  $OUT_DIR/jitinit.o  $OUT_DIR/pycomp.o  $OUT_DIR/pyjit.o $OUT_DIR/cee.o $OUT_DIR/ipycomp.o $OUT_DIR/bridge.o CoreCLR/bin/obj/Linux.x64.Debug/src/jit/dll/libClrJit.a CoreCLR/bin/obj/Linux.x64.Debug/src/gcinfo/lib/libgcinfo.a  CoreCLR/bin/obj/Linux.x64.Debug/src/utilcode/staticnohost/libutilcodestaticnohost.a CoreCLR/bin/obj/Linux.x64.Debug/src/dlls/mscorrc/full/libmscorrc_debug.a CoreCLR/bin/obj/Linux.x64.Debug/src/nativeresources/libnativeresourcestring.a CoreCLR/bin/obj/Linux.x64.Debug/src/pal/src/libcoreclrpal.a CoreCLR/bin/obj/Linux.x64.Debug/src/palrt/libpalrt.a  -lstdc++ -lc -lm  -lgcc_s -lgcc -lc   -lrt -ldl -luuid -lpthread -lutil   -lunwind-x86_64 -lunwind -Wl,-z,noexecstack


clang++-3.5 -DPLATFORM_UNIX=1 -std=c++11 Test/Test.cpp -o Test/test.o -fPIC -g -D_TARGET_AMD64_=1  -IPyjion/ $PY_INC_DIRS Pyjion/pyjion.so Python/libpython3.6m.a -ldl -lpthread -lutil

clang++-3.5 -DPLATFORM_UNIX=1 -std=c++11 Tests/Tests.cpp Tests/test_emission.cpp Tests/test_inference.cpp Tests/testing_util.cpp -o Tests/tests.o -fPIC -g -D_TARGET_AMD64_=1 -ITests/Catch/include/ -IPyjion/ $PY_INC_DIRS Pyjion/pyjion.so Python/libpython3.6m.a -ldl -lpthread -lutil

