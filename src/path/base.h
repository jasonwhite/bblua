/**
 * Copyright (c) Jason White
 *
 * MIT License
 *
 * Description:
 * Chooses a platform-appropriate path implementation.
 */
#pragma once

#include <stddef.h> // For size_t
#include <string.h> // for strlen
#include <string>
#include <vector>

#include "lua.hpp"

/**
 * Helper struct for representing a split path.
 */
template<class T>
struct Split {
    T head;
    T tail;
};

template<class PathImpl>
class BasePath {
public:

    BasePath() : path(NULL), length(0) {}
    BasePath(const std::string& s) : path(s.data()), length(s.length()) {}
    BasePath(const char* path) : path(path), length(strlen(path)) {}
    BasePath(const char* path, size_t length) : path(path), length(length) {}

    const char* path;
    size_t length;

    /**
     * Returns true if the given character is a path separator.
     */
    static bool isSep(char c);

    /**
     * Compares with another path for equality.
     */
    int cmp(const PathImpl& rhs) const;

    /**
     * Returns the length of the root portion of the path.
     *
     * On Posix, simply returns "/" if the path starts with it.
     *
     * On Windows, returns a matching "X:\", "\\server\share", or
     * "\\?\UNC\server\share".
     *
     * If the path is not rooted, returns a path of length 0.
     */
    virtual size_t rootLength() const = 0;

    /**
     * Returns a split where the head is the root part of the path and the tail
     * is the rest of the path.
     *
     * For example, on Windows, "C:\foo" returns ("C:\", "foo").
     *
     * The concatenation of these two paths yields the original path.
     */
    Split<PathImpl> splitRoot() const;

    PathImpl root() const {
        return splitRoot().head;
    }

    /**
     * Returns true if the path is just a root and nothing else.
     */
    bool isRoot() const {
        return length > 0 && rootLength() == length;
    }

    /**
     * Returns true if the given path is absolute.
     */
    bool isabs() const {
        return rootLength() > 0;
    }

    /**
     * Gets the directory name of the path.
     */
    PathImpl dirname() const {
        return split().head;
    }

    /**
     * Gets the name of just the file name in the path.
     */
    PathImpl basename() const {
        return split().tail;
    }

    /**
     * Returns a copy of the path as a string.
     */
    std::string copy() const {
        return std::string(path, length);
    }

    /**
     * Splits a path such that the head is the parent directory (empty if none) and
     * the tail is the basename of the file path.
     */
    Split<PathImpl> split() const;

    /**
     * Splits a path into an extension.
     */
    Split<PathImpl> splitExtension() const;

    /**
     * Returns a list of the path components.
     */
    std::vector<PathImpl> components() const;
    void components(std::vector<PathImpl>& v) const;
    int components(lua_State* L) const;

    /**
     * Normalizes the path such that "." and ".." are resolved. Superfluous
     * directory separators are also removed.
     */
    std::string norm() const;
    void norm(std::string& buf) const;

    /**
     * Joins this path to the end of the given buffer.
     */
    void join(std::string& buf) const;

    /**
     * Returns true if the path is equal to ".".
     */
    bool isDot() const {
        return length == 1 && path[0] == '.';
    }

    /**
     * Returns true if the path is equal to "..".
     */
    bool isDotDot() const {
        return length == 2 && path[0] == '.' && path[1] == '.';
    }
};

template<class PathImpl>
int BasePath<PathImpl>::cmp(const PathImpl& rhs) const {
    ptrdiff_t lengthDiff = (ptrdiff_t)length - (ptrdiff_t)rhs.length;

    if (lengthDiff < 0)
        return -1;
    else if (lengthDiff > 0)
        return 1;

    int result = 0;
    for (size_t i = 0; i < length; ++i) {
        result = PathImpl::cmp(path[i], rhs.path[i]);
        if (result != 0)
            break;
    }

    return result;
}

template<class PathImpl>
Split<PathImpl> BasePath<PathImpl>::splitRoot() const {
    size_t r = rootLength();

    Split<PathImpl> s;
    s.head = PathImpl(path, r);
    s.tail = PathImpl(path + r, length - r);
    return s;
}

template<class PathImpl>
void BasePath<PathImpl>::join(std::string& buf) const {
    if (isabs()) {
        // Path is absolute, reset the buffer length
        buf.clear();
    }
    else {
        // Path is relative, add path separator if necessary.
        size_t len = buf.size();
        if (len > 0 && !PathImpl::isSep(buf[len-1]))
            buf.push_back(PathImpl::defaultSep);
    }

    buf.append(path, length);
}

template<class PathImpl>
Split<PathImpl> BasePath<PathImpl>::split() const {

    // Find the length of the root. We need to ensure we never split inside the
    // root.
    const size_t rootEnd = rootLength();

    // Search backwards for the last path separator
    size_t tail_start = length;

    // Find where the last path component begins.
    while (tail_start > rootEnd) {
        --tail_start;

        if (PathImpl::isSep(path[tail_start])) {
            ++tail_start;
            break;
        }
    }

    // Trim off the path separator(s)
    size_t head_end = tail_start;
    while (head_end > rootEnd) {
        --head_end;

        if (!PathImpl::isSep(path[head_end])) {
            ++head_end;
            break;
        }
    }

    Split<PathImpl> s;
    s.head = PathImpl(path, head_end);
    s.tail = PathImpl(path+tail_start, length-tail_start);
    return s;
}

template<class PathImpl>
Split<PathImpl> BasePath<PathImpl>::splitExtension() const {
    size_t base = length;

    // Find the base name
    while (base > 0) {
        --base;
        if (PathImpl::isSep(path[base])) {
            ++base;
            break;
        }
    }

    // Skip past initial dots
    while (base < length && path[base] == '.')
        ++base;

    // Skip past non-dots
    while (base < length && path[base] != '.')
        ++base;

    Split<PathImpl> s;
    s.head = PathImpl(path, base);
    s.tail = PathImpl(path+base, length-base);
    return s;
}

template<class PathImpl>
std::vector<PathImpl> BasePath<PathImpl>::components() const {
    std::vector<PathImpl> v;
    components(v);
    return v;
}

template<class PathImpl>
void BasePath<PathImpl>::components(std::vector<PathImpl>& v) const {
    Split<PathImpl> s = split();

    if (s.head.isRoot() && !s.tail.length) {
        v.push_back(s.head);
        return;
    }

    if (s.head.length)
    {
        s.head.components(v);

        if (s.tail.length)
            v.push_back(s.tail);
    }
    else if (s.tail.length) {
        v.push_back(s.tail);
    }
}

template<class PathImpl>
int BasePath<PathImpl>::components(lua_State* L) const {
    Split<PathImpl> s = split();

    if (s.head.isRoot() && !s.tail.length) {
        lua_pushlstring(L, s.head.path, s.head.length);
        return 1;
    }

    if (s.head.length)
    {
        int n = s.head.components(L);
        if (s.tail.length) {
            lua_pushlstring(L, s.tail.path, s.tail.length);
            return n + 1;
        }

        return n;
    }
    else if (s.tail.length) {
        lua_pushlstring(L, s.tail.path, s.tail.length);
        return 1;
    }

    return 0;
}

template<class PathImpl>
std::string BasePath<PathImpl>::norm() const {
    std::string buf;
    norm(buf);
    return buf;
}

template<class PathImpl>
void BasePath<PathImpl>::norm(std::string& buf) const {

    // Stack of path components that we build up.
    std::vector<PathImpl> stack;

    for (auto&& c: components()) {
        if (c.isDot()) {
            // Filter out "." path components
            continue;
        }
        else if (c.isDotDot() && !stack.empty() && !stack.back().isDotDot()) {
            if (!stack.back().isabs())
                stack.pop_back();
        }
        else {
            stack.push_back(c);
        }
    }

    if (stack.empty()) {
        PathImpl(".").join(buf);
    }
    else {
        for (auto&& c: stack)
            c.join(buf);
    }

    // Normalize path separators. This should get optimized out for Posix paths.
    for (auto& ch: buf) {
        if (PathImpl::isSep(ch) && ch != PathImpl::defaultSep)
            ch = PathImpl::defaultSep;
    }
}
