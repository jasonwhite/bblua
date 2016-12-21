/**
 * Copyright (c) Jason White
 *
 * MIT License
 *
 * Description:
 * Helper functions for getting global objects for use by C/C++.
 */
#pragma once

#include "threadpool.h"
#include "dircache.h"

namespace lua_globals {

/**
 * Returns the global thread pool object from the given Lua context. Note that
 * this is stored as light user data in a global variable. If the variable is
 * removed, not set, or overwritten, an error is thrown and this function never
 * returns.
 */
ThreadPool& threadPool(lua_State* L);

/**
 * Like threadPool, but returns the directory cache object.
 */
DirCache& dirCache(lua_State* L);

}
