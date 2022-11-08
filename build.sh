#!/usr/bin/env bash

mkdir -p build
cd build
clang -g -O0 -ferror-limit=10 -std=c11 -fshow-column -g -o ika -LC ../src/*.c ../src/rt/*.c ../lib/*.c ../src/backend/*.c
cd ..
