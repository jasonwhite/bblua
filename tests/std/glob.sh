#!/bin/bash -e
# Copyright (c) 2016 Jason White
# MIT License

# Create the directory structure
tempdir=$(mktemp -d)

teardown() {
    rm -rf -- "$tempdir"
}

# Cleanup on exit
trap teardown 0

script=$(pwd)/glob.lua

cd $tempdir

mkdir -- "a" \
         "b" \
         "c"

# Create nested sub directories
mkdir -- "c/1" \
         "c/2" \
         "c/3"

touch -- "a/foo.c" \
         "a/foo.h" \
         "b/bar.c" \
         "b/bar.h" \
         "c/baz.h" \
         "c/1/foo.cc" \
         "c/2/bar.cc" \
         "c/3/baz.cc"

button-lua $script -o /dev/null
