/**
 * Copyright (c) Jason White
 *
 * MIT License
 *
 * Description:
 * Main program logic.
 */

#ifdef _WIN32
#   define _CRT_SECURE_NO_WARNINGS
#endif

#include <string.h>
#include <stdio.h>
#include <string>

#include "button-lua.h"
#include "rules.h"
#include "path.h"
#include "lua_path.h"
#include "embedded.h"
#include "glob.h"
#include "deps.h"
#include "dircache.h"
#include "threadpool.h"

namespace {

const char* usage = "Usage: button-lua <script> [-o output] [args...]\n";

struct Options
{
    const char* script;
    const char* output;
};

struct Args
{
    int n;
    char** argv;
};

/**
 * Parses command line arguments. Returns true if successful.
 */
bool parse_args(Options &opts, Args &args)
{
    if (args.n > 0) {
        opts.script = args.argv[0];
        --args.n; ++args.argv;

        if (args.n > 0 && strcmp(args.argv[0], "-o") == 0) {
            if (args.n > 1)
                opts.output = args.argv[1];
            else
                return false;

            args.n -= 2;
            args.argv += 2;
        }
        else {
            opts.output = NULL;
        }

        return true;
    }

    return false;
}

void print_error(lua_State* L) {
    printf("Error: %s\n", lua_tostring(L, -1));
}

int rule(lua_State* L) {
    buttonlua::Rules* rules = (buttonlua::Rules*)lua_touserdata(L, lua_upvalueindex(1));
    if (rules)
        rules->add(L);
    return 0;
}

int publish_input(lua_State* L) {
    ImplicitDeps* deps = (ImplicitDeps*)lua_touserdata(L, lua_upvalueindex(1));

    size_t len;
    const char* path = luaL_checklstring(L, 1, &len);

    if (deps)
        deps->addInput(path, len);

    return 0;
}

}

namespace buttonlua {

int init(lua_State* L) {

    // Initialize the standard library
    luaL_openlibs(L);

    luaL_requiref(L, "path", luaopen_path, 1);
    lua_pop(L, 1);

    luaL_requiref(L, "winpath", luaopen_winpath, 1);
    lua_pop(L, 1);

    luaL_requiref(L, "posixpath", luaopen_posixpath, 1);
    lua_pop(L, 1);

    lua_pushcfunction(L, lua_glob);
    lua_setglobal(L, "glob");

    lua_getglobal(L, "package");
    lua_getfield(L, -1, "searchers");
    if (lua_type(L, -1) == LUA_TTABLE) {
        // Remove the last entry.
        lua_pushnil(L);
        lua_rawseti(L, -2, 4);

        // Replace the C package loader with our embedded script loader. This
        // kills two birds with one stone:
        //  1. The C package loader can include a module that can alter global
        //     state. Thus, this functionality must be disabled.
        //  2. Adding the embedded script searcher in the correct position.
        //     Scripts on disk should have a higher priority of getting loaded.
        //     This helps with debugging and allows the user to override
        //     functionality if needed.
        lua_pushcfunction(L, embedded_searcher);
        lua_rawseti(L, -2, 3);
    }
    lua_pop(L, 2); // Pop package.searchers and package

    // Run the embedded initialization script
    if (load_init(L) || lua_pcall(L, 0, LUA_MULTRET, 0)) {
        print_error(L);
        return 1;
    }

    return 0;
}

int execute(lua_State* L, int argc, char** argv) {

    Options opts;
    Args args = {argc-1, argv+1};

    if (!parse_args(opts, args)) {
        fputs(usage, stderr);
        return 1;
    }

    // Set SCRIPT_DIR to the script's directory.
    Path dirname = Path(opts.script).dirname();
    lua_pushlstring(L, dirname.path, dirname.length);
    lua_setglobal(L, "SCRIPT_DIR");

    if (luaL_loadfile(L, opts.script) != LUA_OK) {
        print_error(L);
        return 1;
    }

    FILE* output;

    if (!opts.output || strcmp(opts.output, "-") == 0)
        output = stdout;
    else
        output = fopen(opts.output, "w");

    if (!output) {
        perror("Failed to open output file");
        return 1;
    }

    ImplicitDeps deps;
    ThreadPool pool; // TODO: Allow setting pool size from command line
    Rules rules(output);
    DirCache dirCache(&deps);

    lua_pushlightuserdata(L, &dirCache);
    lua_setglobal(L, "__DIR_CACHE");

    lua_pushlightuserdata(L, &pool);
    lua_setglobal(L, "__THREAD_POOL");

    // Register publish_input() function
    lua_pushlightuserdata(L, &deps);
    lua_pushcclosure(L, publish_input, 1);
    lua_setglobal(L, "publish_input");

    // Register rule() function
    lua_pushlightuserdata(L, &rules);
    lua_pushcclosure(L, rule, 1);
    lua_setglobal(L, "rule");

    // Pass along the rest of the command line arguments to the Lua script.
    for (int i = 0; i < args.n; ++i)
        lua_pushstring(L, args.argv[i]);

    if (lua_pcall(L, args.n, LUA_MULTRET, 0) != LUA_OK) {
        print_error(L);
        return 1;
    }

    // Shutdown
    if (load_shutdown(L) || lua_pcall(L, 0, LUA_MULTRET, 0)) {
        print_error(L);
        return 1;
    }

    return 0;
}

}
