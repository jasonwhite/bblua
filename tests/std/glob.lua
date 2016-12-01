--[[
Copyright 2016 Jason White. MIT license.

Description:
Tests globbing.
]]

local function equal(t1, t2)
    table.sort(t1)
    table.sort(t2)

    if #t1 ~= #t2 then
        local msg = string.format(
            "Tables are not of equal length (%d != %d)", #t1, #t2)
        return false, msg
    end

    for i,v in ipairs(t1) do
        if v ~= t2[i] then
            local msg = string.format(
                "Tables are not equal ('%s' != '%s')", v, t2[i])
            return false, msg
        end
    end

    return true
end

-- Don't prepend SCRIPT_DIR to glob paths.
SCRIPT_DIR = nil

assert(equal(
    glob("*/*.c"),
    {
        "a/foo.c",
        "b/bar.c",
    }
))

assert(equal(
    glob("*/*.[ch]"),
    {
        "a/foo.c",
        "a/foo.h",
        "b/bar.c",
        "b/bar.h",
        "c/baz.h",
    }
))

assert(equal(
    glob {"*/*.c", "*/*.h"},
    {
        "a/foo.c",
        "a/foo.h",
        "b/bar.c",
        "b/bar.h",
        "c/baz.h",
    }
))

assert(equal(
    glob("*/"),
    {
        "a",
        "b",
        "c",
    }
))

assert(equal(
    glob("**/"),
    {
        "a",
        "b",
        "c",
        "c/1",
        "c/2",
        "c/3",
    }
))

assert(equal(
    glob("**"),
    {
        "a/foo.c",
        "a/foo.h",
        "b/bar.c",
        "b/bar.h",
        "c/baz.h",
        "c/1/foo.cc",
        "c/2/bar.cc",
        "c/3/baz.cc",
    }
))

assert(equal(
    glob("a/**"),
    {
        "a/foo.c",
        "a/foo.h",
    }
))

assert(equal(
    glob("c/**/1/*"),
    {
        "c/1/foo.cc",
    }
))

assert(equal(
    glob("c/**/*/*"),
    {
        "c/1/foo.cc",
        "c/2/bar.cc",
        "c/3/baz.cc",
    }
))

assert(equal(
    glob("**/foo.h"),
    {
        "a/foo.h",
    }
))

assert(equal(
    glob("**/foobar.baz"),
    {
    }
))

SCRIPT_DIR = "a"

assert(equal(
    glob("*.c"),
    {
        "foo.c",
    }
))

assert(equal(
    glob("*"),
    {
        "foo.c",
        "foo.h",
    }
))

SCRIPT_DIR = ""

assert(equal(
    glob("*/*.c"),
    {
        "a/foo.c",
        "b/bar.c",
    }
))
