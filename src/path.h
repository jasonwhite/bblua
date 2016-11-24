/**
 * Copyright (c) Jason White
 *
 * MIT License
 *
 * Description:
 * Chooses a platform-appropriate path implementation.
 */
#pragma once

#include "path/posix.h"
#include "path/windows.h"

#ifdef _WIN32
using Path = WinPath;
#else
using Path = PosixPath;
#endif
