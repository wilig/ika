#!/usr/bin/env bash

mkdir -p build
cd build
clang -ferror-limit=10 -std=c11 -fshow-column -g -o ika -LC ../src/*.c
cd ..
