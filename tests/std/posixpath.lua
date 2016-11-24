--[[
Copyright 2016 Jason White. MIT license.

Description:
Tests the path manipulation functions for Posix.
]]

local path = posixpath;

--[[
    Get the root part of the path.
]]
assert(path.splitroot("/") == "/")
assert(path.splitroot("/foo") == "/")
assert(path.splitroot("foo") == "")

--[[
    path.isabs
]]
assert(path.isabs("/"))
assert(path.isabs("/foo/bar"))
assert(not path.isabs("foo"))
assert(not path.isabs(""))


--[[
    path.join
]]
assert(path.join() == "")
assert(path.join("") == "")
assert(path.join("", "") == "")
assert(path.join("foo") == "foo")
assert(path.join("foo", "bar") == "foo/bar")
assert(path.join("foo", "/bar") == "/bar")
assert(path.join("foo/", "bar") == "foo/bar")
assert(path.join("foo/", nil, "bar") == "foo/bar")
assert(path.join("foo//", "bar") == "foo//bar")
assert(path.join("/", "foo") == "/foo")
assert(path.join("foo", "") == "foo/")
assert(path.join("foo", nil) == "foo")
assert(path.join("", "foo") == "foo")
assert(path.join(nil, "foo") == "foo")


--[[
    path.split
]]

local split_tests = {
    {"", "", "", ""},
    {"/", "/", "", "/"},
    {"foo", "", "foo", "foo"},
    {"/foo", "/", "foo", "/foo"},
    {"foo/bar", "foo", "bar", "foo/bar"},
    {"foo/", "foo", "", "foo/"},
    {"/foo////bar", "/foo", "bar", "/foo/bar"},
    {"////foo////bar", "////foo", "bar", "////foo/bar"},
}

local split_error = 'In path.split("%s"): expected "%s", got "%s"'

for k,v in ipairs(split_tests) do
    local head, tail = path.split(v[1])
    assert(head == v[2], string.format(split_error, v[1], v[2], head))
    assert(tail == v[3], string.format(split_error, v[1], v[3], tail))
    assert(path.join(head, tail) == v[4],
        string.format(split_error, v[1], v[4], path.join(head, tail)))
end


--[[
    path.basename
]]
assert(path.basename("") == "")
assert(path.basename("/") == "")
assert(path.basename("/foo") == "foo")
assert(path.basename("/foo/bar") == "bar")
assert(path.basename("////foo////bar") == "bar")
assert(path.basename("/foo/bar/") == "")

--[[
    path.dirname
]]
assert(path.dirname("") == "")
assert(path.dirname("/") == "/")
assert(path.dirname("/foo") == "/")
assert(path.dirname("/foo/bar") == "/foo")
assert(path.dirname("/foo/bar/") == "/foo/bar")


--[[
    path.splitext
]]

local splitext_tests = {
    {"", "", ""},
    {"/", "/", ""},
    {"foo", "foo", ""},
    {".foo", ".foo", ""},
    {"foo.bar", "foo", ".bar"},
    {"foo/bar.baz", "foo/bar", ".baz"},
    {"foo/.bar.baz", "foo/.bar", ".baz"},
    {"foo/....bar.baz", "foo/....bar", ".baz"},
    {"/....bar", "/....bar", ""},
}

local split_error = 'In path.splitext("%s"): expected "%s", got "%s"'

for k,v in ipairs(splitext_tests) do
    local root, ext = path.splitext(v[1])
    assert(root == v[2], string.format(split_error, v[1], root, v[2]))
    assert(ext  == v[3], string.format(split_error, v[1], ext, v[3]))
    assert(root .. ext == v[1])
end

--[[
    path.getext
]]

assert(path.getext("") == "")
assert(path.getext("/") == "")
assert(path.getext("/foo") == "")
assert(path.getext("/foo.") == ".")
assert(path.getext("/foo.bar") == ".bar")
assert(path.getext("/.foo.bar") == ".bar")

--[[
    path.getext
]]

assert(path.setext("", ".c") == ".c")
assert(path.setext("/", ".c") == "/.c")
assert(path.setext("foo", ".c") == "foo.c")
assert(path.setext("foo.", ".c") == "foo.c")
assert(path.setext("foo.bar", ".c") == "foo.c")
assert(path.setext(".foo.bar", ".c") == ".foo.c")
assert(path.setext(".foo", ".c") == ".foo.c")

--[[
    path.components
]]

local components_tests = {
    {"", {}, ""},
    {"foo", {"foo"}, "foo"},
    {"foo/", {"foo"}, "foo"},
    {"foo/bar", {"foo", "bar"}, "foo/bar"},
    {"foo/bar/", {"foo", "bar"}, "foo/bar"},
    {"/foo/bar/", {"/", "foo", "bar"}, "/foo/bar"},
    {"foo///bar", {"foo", "bar"}, "foo/bar"},
    {"/foo/bar", {"/", "foo", "bar"}, "/foo/bar"},
    {"../foo/bar/baz", {"..", "foo", "bar", "baz"}, "../foo/bar/baz"},
}

local components_error = 'In path.components("%s"): expected %s, got %s'

for _,v in ipairs(components_tests) do
    local components = {path.components(v[1])}

    local got = table.show(components, "")
    local expected = table.show(v[2], "")

    assert(#components == #v[2],
        string.format(components_error, v[1], expected, got))

    for i,c in ipairs(components) do
        assert(c == v[2][i],
            string.format(components_error, v[1], c, v[2][i]))
    end

    local joined = path.join(table.unpack(components))
    assert(joined == v[3],
        string.format('In path.components("%s"): got joined path "%s", expected "%s"',
        v[1], joined, v[3]))
end

--[[
    path.norm
]]
assert(path.norm("") == ".")
assert(path.norm(".") == ".")
assert(path.norm("foo") == "foo")
assert(path.norm("./foo") == "foo")
assert(path.norm("/foo") == "/foo")
assert(path.norm("foo//bar") == "foo/bar")
assert(path.norm("foo/./bar") == "foo/bar")
assert(path.norm("foo/../bar") == "bar")
assert(path.norm("foo/../bar/..") == ".")
assert(path.norm("../foo/../bar") == "../bar")
assert(path.norm("/../foo/../bar") == "/bar")
assert(path.norm("/../../.././bar/") == "/bar")
assert(path.norm("../foo/../bar/") == "../bar")
assert(path.norm("../foo/../bar///") == "../bar")
assert(path.norm("../path/./to//a/../b/c/../../test.txt/") == "../path/to/test.txt")
