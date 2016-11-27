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

#include <mutex>
#include <string>
#include <set>

#include "lua.hpp"

#include "glob.h"
#include "path.h"
#include "dircache.h"

namespace {

/**
 * Compare a character with case sensitivity or not.
 */
template <bool CaseSensitive>
int charCmp(char a, char b) {
    if (CaseSensitive)
        return (int)a - (int)b;
    else
        return (int)tolower(a) - (int)tolower(b);
}

template <bool CaseSensitive>
bool globMatch(Path path, Path pattern) {

    size_t i = 0;

    for (size_t j = 0; j < pattern.length; ++j) {
        switch (pattern.path[j]) {
            case '?': {
                // Match any single character
                if (i == path.length)
                    return false;
                ++i;
                break;
            }

            case '*': {
                // Match 0 or more characters
                if (j+1 == pattern.length)
                    return true;

                // Consume characters while looking ahead for matches
                for (; i < path.length; ++i) {
                    if (globMatch<CaseSensitive>(
                                Path(path.path+i, path.length-i),
                                Path(pattern.path+j+1, pattern.length-j-1)))
                        return true;
                }

                return false;
            }

            case '[': {
                // Match any of the characters that appear in the square brackets
                if (i == path.length) return false;

                // Skip past the opening bracket
                if (++j == pattern.length) return false;

                // Invert the match?
                bool invert = false;
                if (pattern.path[j] == '!') {
                    invert = true;
                    if (++j == pattern.length)
                        return false;
                }

                // Find the closing bracket
                size_t end = j;
                while (end < pattern.length && pattern.path[end] != ']')
                    ++end;

                // No matching bracket?
                if (end == pattern.length) return false;

                // Check each character between the brackets for a match
                bool match = false;
                while (j < end) {
                    // Found a match
                    if (!match && charCmp<CaseSensitive>(path.path[i], pattern.path[j]) == 0) {
                        match = true;
                    }

                    ++j;
                }

                if (match == invert)
                    return false;

                ++i;
                break;
            }

            default: {
                // Match the next character in the pattern
                if (i == path.length || charCmp<CaseSensitive>(path.path[i], pattern.path[j]))
                    return false;
                ++i;
                break;
            }
        }
    }

    // If we ran out of pattern and out of path, then we have a complete match.
    return i == path.length;
}

}

bool globMatch(Path path, Path pattern) {
#ifdef _WIN32
    return globMatch<false>(path, pattern);
#else
    return globMatch<true>(path, pattern);
#endif
}

int lua_glob_match(lua_State* L) {
    size_t len, patlen;
    const char* path = luaL_checklstring(L, 1, &len);
    const char* pattern = luaL_checklstring(L, 2, &patlen);
    lua_pushboolean(L, globMatch(Path(path, len), Path(pattern, patlen)));
    return 1;
}

int lua_glob(lua_State* L) {

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

    std::mutex mutex; // Protects set of paths
    std::set<std::string> paths;

    // Adds a path to the set.
    GlobCallback include = [&] (Path path, bool isDir) {
        (void)isDir;
        std::lock_guard<std::mutex> lock(mutex);
        paths.insert(std::string(path.path, path.length));
    };

    // Removes a path to the set.
    GlobCallback exclude = [&] (Path path, bool isDir) {
        (void)isDir;
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
                        dirCache->glob(root, Path(path+1, len-1), exclude);
                    else
                        dirCache->glob(root, Path(path, len), include);
                }

                lua_pop(L, 1); // Pop path
            }
        }
        else if (type == LUA_TSTRING) {
            path = luaL_checklstring(L, i, &len);

            if (len > 0 && path[0] == '!')
                dirCache->glob(root, Path(path+1, len-1), exclude);
            else
                dirCache->glob(root, Path(path, len), include);
        }
    }

    // Construct the Lua table.
    lua_newtable(L);
    lua_Integer n = 1;

    for (std::set<std::string>::iterator it = paths.begin(); it != paths.end(); ++it) {
        lua_pushlstring(L, it->data(), it->size());
        lua_rawseti(L, -2, n);
        ++n;
    }

    return 1;
}
