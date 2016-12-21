/**
 * Copyright (c) Jason White
 *
 * MIT License
 *
 * Description:
 * Globbing.
 */
#pragma once

#include "path.h"

struct lua_State;

/**
 * Lists files based on a glob expression.
 *
 * Arguments:
 *  - pattern: A pattern string or table of pattern strings
 *
 * Returns: A table of the matching files.
 */
int lua_glob(lua_State* L);
