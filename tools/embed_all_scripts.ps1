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

param (
    [Parameter(Mandatory=$true)][string]$lua
)

# Stop immediately if something fails.
$ErrorActionPreference='Stop'

# We need the absolute path to the Lua executable since we will be changing
# directories.
$lua=(Resolve-Path $lua)

# CD to the root of this project; "tools\embed.lua" needs this to be its
# working directory.
Set-Location (Join-Path $PSScriptRoot '..')

# Generate the C files from the Lua files
foreach ($name in (Get-ChildItem scripts -Include "*.lua" -Recurse -Name)) {
    $outfile = (Join-Path 'src\embedded' ([io.path]::ChangeExtension($name, 'c')))
    $dirname = [io.path]::GetDirectoryName($outfile)
    mkdir -Force -Path $dirname > $null
    Write-Host "Generating '$outfile'..."
    & $lua tools\embed.lua (Join-Path scripts $name) | Out-File $outfile
}
