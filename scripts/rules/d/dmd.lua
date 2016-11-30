--[[
Copyright (c) Jason White. MIT license.

Description:
Generates rules for the DMD compiler.
]]

local rules = require "rules"

local cc = require "rules.cc"

--[[
Helper functions.

TODO: Alter paths based on platform
]]
local function is_d_source(src)
    return path.getext(src) == ".d"
end

local function to_object(objdir, src)
    return path.norm(path.setext(path.join(objdir, src), ".o"))
end

--[[
    Filters for D source files.
]]
local function sources(files)
    local srcs = table.filter(files, is_d_source)

    for i,v in ipairs(srcs) do
        files[i] = path.norm(v)
    end

    return srcs
end

--[[
    Returns a list of objects corresponding to the given list of sources.
]]
local function objects(srcs, objdir)
    local objs = {}

    for _,v in ipairs(srcs) do
        table.insert(objs, to_object(objdir, v))
    end

    return objs
end

--[[
Base metatable
]]
local common = {

    -- Path to DMD
    compiler = {"dmd"};

    -- Extra options
    opts = {"-color=on"};

    -- Path to the bin directory
    bindir = "";

    -- Build all source on the same command line. Otherwise, each source is
    -- compiled separately and finally linked separately. In general, combined
    -- compilation is faster.
    combined = true;

    -- Paths to look for imports
    imports = {};

    -- Paths to look for string imports
    string_imports = {};

    -- Versions to define with '-version='
    versions = {};

    -- Extra compiler and linker options
    compiler_opts = {};
    linker_opts = {};

    -- File dependencies for particular source files. This allows fine-grained
    -- control over dependencies.
    src_deps = {};
}

function common:path()
    return path.norm(path.join(self.bindir, self:basename()))
end

setmetatable(common, {__index = rules.common})

--[[
A binary executable.
]]
local _binary = {}

local _binary_mt = {__index = _binary}

local function is_binary(t)
    return getmetatable(t) == _binary_mt
end

setmetatable(_binary, {__index = common})

--[[
A library. Can be static or dynamic.
]]
local _library = {
    -- Shared library?
    shared = false,
}

local _library_mt = {__index = _library}

local function is_library(t)
    return getmetatable(t) == _library_mt
end

setmetatable(_library, {__index = common})

--[[
A test.
]]
local _test = {}
local _test_mt = {__index = _test}

local function is_test(t)
    return getmetatable(t) == _test_mt
end

setmetatable(_test, {__index = common})


--[[
Generates the low-level rules required to build a generic D library/binary.
]]
function common:rules()
    local objdir = self.objdir or path.norm(path.join("obj", self:basename()))
    self.objdir = objdir;

    local args = table.join(self.prefix, self.compiler, self.opts)

    local compiler_opts = {"-op", "-od".. objdir}

    for _,v in ipairs(self.imports) do
        table.insert(compiler_opts,
            "-I" .. path.norm(path.join(self.scriptdir, v)))
    end

    for _,v in ipairs(self.string_imports) do
        table.insert(compiler_opts,
            "-J" .. path.norm(path.join(self.scriptdir, v)))
    end

    for _,v in ipairs(self.versions) do
        table.insert(compiler_opts, "-version=" .. v)
    end

    table.append(compiler_opts, self.compiler_opts)

    local srcs = sources(self.srcs)
    local objs = objects(srcs, path.join(self.scriptdir, objdir))

    local libs = {}

    for _,dep in ipairs(self.deps) do
        if is_library(dep) then
            table.insert(libs, dep:path())
        elseif cc.is_library(dep) then
            table.insert(libs, dep:path())
        else
            error(string.format(
                "The dependency on '%s' from '%s' is not supported",
                dep.name, self.name
                ),
                0)
        end
    end

    local output = self:output()
    table.append(compiler_opts, "-of" .. output)

    if self.combined then
        local deps = {}
        for i,src in ipairs(srcs) do
            for _,dep in ipairs(self.src_deps[src] or {}) do
                table.insert(deps, path.norm(path.join(self.scriptdir, dep)))
            end

            srcs[i] = path.norm(path.join(self.scriptdir, src))
        end

        -- Combined compilation
        rule {
            inputs  = table.join(srcs, libs, deps),
            task    = {table.join(args, compiler_opts, self.linker_opts, srcs, libs)},
            outputs = {self:path()},
            display = "dmd ".. self:basename(),
        }
    else
        -- Individual compilation
        for i,src in ipairs(srcs) do

            local deps = {}
            for _,v in ipairs(self.src_deps[src] or {}) do
                table.insert(deps, path.norm(path.join(self.scriptdir, v)))
            end

            local src = path.norm(path.join(self.scriptdir, src))
            local obj = objs[i]

            rule {
                inputs  = table.join({src}, deps),
                task    = {table.join(args, compiler_opts,
                    {"-c", src, "-of".. obj})},
                outputs = {obj},
                display = "dmd ".. src,
            }
        end

        rule {
            inputs = table.join(objs, libs),
            task = {table.join(args, linker_opts, objs, libs)},
            outputs = {self:path()},
            display = "dmd ".. self:basename(),
        }
    end
end

function common:output()
    return path.norm(path.join(self.bindir, self:basename()))
end

function _library:basename()
    local name = common.basename(self)

    if self.shared then
        return "lib".. name .. ".so"
    else
        return "lib".. name .. ".a"
    end
end

function _library:output()
    return self:basename()
end

function _library:rules()
    if self.shared then
        self.compiler_opts = table.join(self.compiler_opts, "-fPIC")
        self.linker_opts = table.join(self.linker_opts, {
            "-shared", "-defaultlib=libphobos2.so"
            })
    else
        self.linker_opts = table.join(self.linker_opts, "-lib")
    end

    common.rules(self)
end

function _library:path()
    return path.norm(path.join(self.objdir, self:basename()))
end

function _test:rules()
    self.compiler_opts = table.join(self.compiler_opts, "-unittest")

    common.rules(self)

    local test_runner = self:path()

    rule {
        inputs  = {test_runner},
        task    = {{path.join(".", test_runner)}},
        outputs = {},
        display = "dmd ".. self:basename(),
    }
end

local template = {
    binary = function(opts)
        return setmetatable(opts, _binary_mt)
    end,

    library = function(opts)
        return setmetatable(opts, _library_mt)
    end,

    test = function(opts)
        return setmetatable(opts, _test_mt)
    end,
}

local function binary(opts, base)
    base = base or _binary_mt
    setmetatable(opts, base)
    return rules.add(opts)
end

local function library(opts, base)
    base = base or _library_mt
    setmetatable(opts, base)
    return rules.add(opts)
end

local function test(opts, base)
    base = base or _test_mt
    setmetatable(opts, base)
    return rules.add(opts)
end

return {
    common = common,

    is_binary = is_binary,
    is_library = is_library,
    is_test = is_test,

    binary = binary,
    library = library,
    test = test,

    template = template,
}
