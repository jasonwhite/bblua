/**
 * Copyright (c) Jason White
 *
 * MIT License
 */

#include "lua.hpp"

#include "lua_globals.h"

namespace lua_globals {

ThreadPool& threadPool(lua_State* L) {
    // Get the directory cache object.
    lua_getglobal(L, "__THREAD_POOL");
    ThreadPool* threadPool = (ThreadPool*)lua_topointer(L, -1);
    lua_pop(L, 1); // Pop __THREAD_POOL

    if (!threadPool) {
        // This would probably only happen if someone messes with this global
        // variable in a Lua script.
        luaL_error(L, "__THREAD_POOL does not point to any object");

        // Never returns.
    }

    return *threadPool;
}

DirCache& dirCache(lua_State* L) {
    // Get the directory cache object.
    lua_getglobal(L, "__DIR_CACHE");
    DirCache* dirCache = (DirCache*)lua_topointer(L, -1);
    lua_pop(L, 1); // Pop __DIR_CACHE

    if (!dirCache) {
        // This would probably only happen if someone messes with this global
        // variable in a Lua script.
        luaL_error(L, "__DIR_CACHE does not point to any object");

        // Never returns.
    }

    return *dirCache;
}

}
