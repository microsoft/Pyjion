#!/bin/bash

g++ -std=c++11 intrins.cpp -I../Python/Include/ -I../Python/  -o intrins.o -S

g++ -std=c++11 absvalue.cpp -I../Python/Include/ -I../Python/  -o absvalue.o -S


g++ -std=c++11 absint.cpp -I../Python/Include/ -I../Python/  -o absint.o -S

g++ -std=c++11 jitinit.cpp -I../Python/Include/ -I../Python/  -o jitinit.o -S

g++ -std=c++11 absint.cpp -I../Python/Include/ -I../Python/  -o absint.o -S


#g++ -std=c++11 pycomp.cpp -o pycomp.o -S -I../CoreCLR/src/inc -I../CoreCLR/src/palrt/inc -I../CoreCLR/src/pal/inc -I../CoreCLR/src/pal/inc/rt -I../CoreCLR/src/pal/prebuilt/inc -D_TARGET_AMD64_=1
