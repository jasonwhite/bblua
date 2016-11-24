/**
 * Copyright (c) Jason White
 *
 * MIT License
 *
 * Description:
 * File path manipulation.
 */
#include "path/posix.h"

#include "lua.hpp"

int PosixPath::cmp(char a, char b) {
    if (isSep(a) && isSep(b))
        return 0;

    return (int)a - (int)b;
}

size_t PosixPath::rootLength() const {
    return length >= 1 && isSep(path[0]);
}
