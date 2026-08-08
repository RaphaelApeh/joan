#!/usr/bin/env bash
set -e

OPT=""

if [["$1" == "debug"]]; then
    OPT="-ggdb -DJOAN_DEBUG"
fi

CC = "gcc"
if [["$2" == "clang"]]; then
    CC = "clang"
fi

cd src

SRC = $(find . -maxdepth 1 -name "*.c" | -name "main.c")

$CC $OPT -O2 -Wall -DJN_BUILD_DLL -Wno-unused-function -Wno-pointer-sign -Wno-unused-variable    \
    -I../include -c $SRC

$CC $OPT -shared *.o -o libjoan.so

$CC -O2 -Wall -Wno-unused-function -Wno-pointer-sign -Wno-unused-variable \
    -I../include -c main.c

$CC main.o -o joan -L. -ljoan -lm

ar rcs libjoan.a *.o

rm -f *.o

cd ..

# TODO