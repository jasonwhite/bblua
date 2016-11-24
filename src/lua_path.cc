/**
 * Copyright (c) Jason White
 *
 * MIT License
 *
 * Description:
 * Path manipulation module.
 */
#include "lua_path.h"

#include "path.h"

#include "lua.hpp"

template<class Path>
static int path_splitroot(lua_State* L) {
    size_t len;
    const char* path = luaL_checklstring(L, 1, &len);

    const Split<Path> s = Path(path, len).splitRoot();
    lua_pushlstring(L, s.head.path, s.head.length);
    lua_pushlstring(L, s.tail.path, s.tail.length);
    return 2;
}

template<class Path>
static int path_isabs(lua_State* L) {
    size_t len;
    const char* path = luaL_checklstring(L, 1, &len);
    lua_pushboolean(L, Path(path, len).isabs());
    return 1;
}

template<class Path>
static int path_join(lua_State* L) {

    int argc = lua_gettop(L);

    luaL_Buffer b;
    luaL_buffinit(L, &b);

    for (int i = 1; i <= argc; ++i) {
        if (lua_isnil(L, i))
            continue;

        size_t len;
        const char* path = luaL_checklstring(L, i, &len);

        if (Path(path, len).isabs()) {
            // Path is absolute, reset the buffer length
            b.n = 0;
        }
        else {
            // Path is relative, add path separator if necessary.
            if (b.n > 0 && !Path::isSep(b.b[b.n-1]))
                luaL_addchar(&b, Path::defaultSep);
        }

        luaL_addlstring(&b, path, len);
    }

    luaL_pushresult(&b);
    return 1;
}

template<class Path>
static int path_split(lua_State* L) {

    size_t len;
    const char* path = luaL_checklstring(L, 1, &len);

    Split<Path> s = Path(path, len).split();

    lua_pushlstring(L, s.head.path, s.head.length);
    lua_pushlstring(L, s.tail.path, s.tail.length);

    return 2;
}

template<class Path>
static int path_basename(lua_State* L) {
    size_t len;
    const char* path = luaL_checklstring(L, 1, &len);

    Split<Path> s = Path(path, len).split();
    lua_pushlstring(L, s.tail.path, s.tail.length);
    return 1;
}

template<class Path>
static int path_dirname(lua_State* L) {
    size_t len;
    const char* path = luaL_checklstring(L, 1, &len);

    Split<Path> s = Path(path, len).split();
    lua_pushlstring(L, s.head.path, s.head.length);
    return 1;
}

template<class Path>
static int path_splitext(lua_State* L) {

    size_t len;
    const char* path = luaL_checklstring(L, 1, &len);

    Split<Path> s = Path(path, len).splitExtension();

    lua_pushlstring(L, s.head.path, s.head.length);
    lua_pushlstring(L, s.tail.path, s.tail.length);

    return 2;
}

template<class Path>
static int path_getext(lua_State* L) {
    size_t len;
    const char* path = luaL_checklstring(L, 1, &len);

    Split<Path> s = Path(path, len).splitExtension();

    lua_pushlstring(L, s.tail.path, s.tail.length);
    return 1;
}

template<class Path>
static int path_setext(lua_State* L) {
    size_t pathlen, extlen;
    const char* path = luaL_checklstring(L, 1, &pathlen);
    const char* ext = luaL_checklstring(L, 2, &extlen);

    Split<Path> s = Path(path, pathlen).splitExtension();

    lua_pushlstring(L, s.head.path, s.head.length);
    lua_pushlstring(L, ext, extlen);
    lua_concat(L, 2);

    return 1;
}

template<class Path>
static int path_components(lua_State* L) {

    size_t len;
    if (const char* str = luaL_checklstring(L, 1, &len))
        return Path(str, len).components(L);

    return 0;
}

template<class Path>
static int path_norm(lua_State* L) {

    size_t len;
    const char* path = luaL_checklstring(L, 1, &len);

    auto buf = Path(path, len).norm();

    lua_pushlstring(L, buf.data(), buf.length());

    return 1;
}

static const luaL_Reg pathlib_posix[] = {
    {"splitroot", path_splitroot<PosixPath>},
    {"isabs", path_isabs<PosixPath>},
    {"join", path_join<PosixPath>},
    {"split", path_split<PosixPath>},
    {"basename", path_basename<PosixPath>},
    {"dirname", path_dirname<PosixPath>},
    {"splitext", path_splitext<PosixPath>},
    {"getext", path_getext<PosixPath>},
    {"setext", path_setext<PosixPath>},
    {"components", path_components<PosixPath>},
    {"norm", path_norm<PosixPath>},
    {NULL, NULL}
};

static const luaL_Reg pathlib_win[] = {
    {"splitroot", path_splitroot<WinPath>},
    {"isabs", path_isabs<WinPath>},
    {"join", path_join<WinPath>},
    {"split", path_split<WinPath>},
    {"basename", path_basename<WinPath>},
    {"dirname", path_dirname<WinPath>},
    {"splitext", path_splitext<WinPath>},
    {"getext", path_getext<WinPath>},
    {"setext", path_setext<WinPath>},
    {"components", path_components<WinPath>},
    {"norm", path_norm<WinPath>},
    {NULL, NULL}
};

int luaopen_path(lua_State* L) {
#ifdef _WIN32
    luaL_newlib(L, pathlib_win);
#else
    luaL_newlib(L, pathlib_posix);
#endif
    return 1;
}

int luaopen_posixpath(lua_State* L) {
    luaL_newlib(L, pathlib_posix);
    return 1;
}

int luaopen_winpath(lua_State* L) {
    luaL_newlib(L, pathlib_win);
    return 1;
}
