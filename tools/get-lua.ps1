# Copyright (c) 2016 Jason White
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# Change these when updating the version of Lua.
$lua_version = '5.3.3'
$lua_sha1    = 'A0341BC3D1415B814CC738B2EC01AE56045D64EF'
$lua_url     = "https://www.lua.org/ftp/lua-$lua_version.tar.gz"

$lua_archive = "lua-$lua_version.tar.gz"

# Stop immediately if something fails.
$ErrorActionPreference='Stop'

# Download Lua if we haven't already done it.
if (!(Test-Path $lua_archive)) {
    Invoke-WebRequest $lua_url -OutFile $lua_archive
}

# Verify the hash
$hash = (Get-FileHash $lua_archive -Algorithm SHA1).Hash
if ($hash -ne $lua_sha1) {
    throw "Invalid SHA1 hash for $lua_archive. Got '$hash', expected '$lua_sha1'."
}

# We need 7zip on our PATH so we don't have to specify the entire command
# below. PowerShell is horrible at passing quoted arguments through to CMD.
$env:Path += ';C:\Program Files\7-Zip'

# Use cmd.exe because PowerShell is horrible at piping processes.
& cmd.exe "/C 7z x $lua_archive -so | 7z x -aoa -si -ttar lua-$lua_version -ovs"
