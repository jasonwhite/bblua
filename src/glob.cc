/**
 * Copyright (c) Jason White
 *
 * MIT License
 *
 * Description:
 * Globbing.
 */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <algorithm>
#include <mutex>
#include <string>
#include <set>

#include "lua.hpp"

#include "glob.h"
#include "path.h"
#include "lua_globals.h"

int lua_glob(lua_State* L) {

    DirCache& dirCache = lua_globals::dirCache(L);
    ThreadPool& pool = lua_globals::threadPool(L);

    std::mutex mutex;
    std::set<std::string> paths;

    // Adds a path to the set.
    MatchCallback include = [&] (Path path) {
        std::lock_guard<std::mutex> lock(mutex);
        paths.emplace(path.path, path.length);
    };

    // Removes a path to the set.
    MatchCallback exclude = [&] (Path path) {
        std::lock_guard<std::mutex> lock(mutex);
        paths.erase(std::string(path.path, path.length));
    };

    int argc = lua_gettop(L);

    lua_getglobal(L, "SCRIPT_DIR");

    const char* scriptDir = lua_tostring(L, -1);

    if (!scriptDir || scriptDir[0] == '\0')
        scriptDir = ".";

    Path root = scriptDir;

    lua_pop(L, 1); // Pop SCRIPT_DIR

    size_t len;
    const char* path;

    for (int i = 1; i <= argc; ++i) {
        const int type = lua_type(L, i);

        if (type == LUA_TTABLE) {
            for (int j = 1; ; ++j) {
                lua_rawgeti(L, i, j);
                if (lua_type(L, -1) == LUA_TNIL) {
                    lua_pop(L, 1);
                    break;
                }

                path = lua_tolstring(L, -1, &len);
                if (path) {
                    if (len > 0 && path[0] == '!')
                        dirCache.glob(root, Path(path+1, len-1), exclude, &pool);
                    else
                        dirCache.glob(root, Path(path, len), include, &pool);
                }

                lua_pop(L, 1); // Pop path
            }
        }
        else if (type == LUA_TSTRING) {
            path = luaL_checklstring(L, i, &len);

            if (len > 0 && path[0] == '!')
                dirCache.glob(root, Path(path+1, len-1), exclude, &pool);
            else
                dirCache.glob(root, Path(path, len), include, &pool);
        }
    }

    // Construct the Lua table.
    lua_newtable(L);
    lua_Integer n = 1;

    // Anything left over in includes cannot also be in the excludes.
    for (auto&& p: paths) {
        lua_pushlstring(L, p.data(), p.length());
        lua_rawseti(L, -2, n);
        ++n;
    }

    return 1;
}
