[travis-ci-badge]: https://travis-ci.org/jasonwhite/button-lua.svg?branch=master
[appveyor-badge]: https://ci.appveyor.com/api/projects/status/ytgd4d2uree1cb7h/branch/master?svg=true

# Lua Build Descriptions for Button

[![Build Status][travis-ci-badge]](https://travis-ci.org/jasonwhite/button-lua)
[![Build status][appveyor-badge]](https://ci.appveyor.com/project/jasonwhite/button-lua/branch/master)

A tool to generate build descriptions from Lua scripts that are suitable for
input to [Button][].

[Button]: https://github.com/jasonwhite/button

## Example

```lua
local cc = require "rules.cc"

cc.binary {
    name = "foobar",
    srcs = {"foo.c", "bar.c"},
}
```

If this script is named `BUILD.lua`, we can generate the low-level build
description by running

    button-lua BUILD.lua -o button.json

See also [BUILD.lua](/BUILD.lua) for this repository for a real-world example.

## Building it

Simply run:

    make

This will create a self-contained executable named `button-lua`.

## License

[MIT License](/LICENSE.md)
