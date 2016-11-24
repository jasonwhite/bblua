/**
 * Copyright (c) Jason White
 *
 * MIT License
 *
 * Description:
 * File path manipulation for Windows.
 *
 * Reference:
 * https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247.aspx
 */

#include <cctype>
#include <cstring>
#include <algorithm>

#include "path/windows.h"
#include "lua.hpp"

int WinPath::cmp(char a, char b) {
    if (isSep(a) && isSep(b))
        return 0;

    return (int)tolower(a) - (int)tolower(b);
}

/**
 * Be sure to compare the simplicity of this function with that of
 * PosixPath::rootLength(). Path manipulation on Windows is goddamn insane.
 */
size_t WinPath::rootLength() const {
    if (length >= 4 && strncmp(path, "\\\\?\\", 4) == 0) {
        // Path starts with "\\?\". This is used to allow paths longer than 260
        // characters (but only for UTF-16 Windows API functions). Such elegant
        // design.

        // Either a drive or a UNC path can follow this. We need to include
        // those in the absolute part as well.
        if (length >= 7 && path[5] == ':' && isSep(path[6])) {
            // Path is of the form "\\?\C:\". 7 characters long.
            return 7;
        }
        else if (length >= 8 && strncmp(path+4, "UNC\\", 4) == 0) {
            // Path is of the form "\\?\UNC\server\share".
            size_t i = 8;

            // Skip past the server name.
            while (i < length && !isSep(path[i])) ++i;

            if (i > 8 && i < length) {
                ++i; // Skip past the path separator

                // Skip past the share name
                while (i < length && !isSep(path[i])) ++i;

                return i;
            }
        }

        // At the very least, include "\\?\".
        return 4;
    }
    else if (length >= 4 && strncmp(path, "\\\\.\\", 4) == 0) {
        // A device name follows. We need to include that in the absolute part
        // as well.
        size_t i = 4;

        // Skip past the device name.
        while (i < length && !isSep(path[i])) ++i;

        return i;
    }
    else if (length >= 4 && isSep(path[0]) && isSep(path[1])) {
        // Path is a UNC path (e.g., "\\server\share").
        size_t i = 2;

        // Skip past the server name
        while (i < length && !isSep(path[i])) ++i;

        if (i > 2 && i < length) {
            ++i; // Skip past the path separator

            // Skip past the share name
            while (i < length && !isSep(path[i])) ++i;

            return i;
        }
    }
    else if (length >= 3 && path[1] == ':' && isSep(path[2])) {
        // Path starts with "X:\" or "X:/".
        return 3;
    }
    else if (length >= 1 && isSep(path[0])) {
        // Path starts with a single path separator (and nothing more).
        if (length >= 2) {
            if (!isSep(path[1]))
                return 1;
        }
        else {
            return 1;
        }
    }

    return 0;
}
