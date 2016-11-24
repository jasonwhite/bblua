/**
 * Copyright (c) Jason White
 *
 * MIT License
 */

#ifdef _WIN32
#   include <windows.h>
#else
#   include <dirent.h>
#   include <sys/stat.h>
#   include <fcntl.h>
#endif // _WIN32

#include <algorithm>
#include <utility>

#include "dircache.h"
#include "glob.h"
#include "path.h"
#include "deps.h"

bool operator<(const DirEntry& a, const DirEntry& b) {
    return std::tie(a.name, a.isDir) < std::tie(b.name, b.isDir);
}

namespace {

/**
 * Returns true if a NULL-terminated path is "." or "..".
 */
bool isDotOrDotDot(const char* p) {
    return (*p++ == '.' && (*p == '\0' || (*p++ == '.' && *p == '\0')));
}

/**
 * Returns a list of the files in a directory.
 */
DirEntries dirEntries(const std::string& path) {

    DirEntries entries;

#ifdef _WIN32

    // FIXME: Use the UTF-16 version of the API. We are using UTF-8 everywhere
    // internally, so we need to convert.
    //
    // Use the ANSI version of the API for now.
    WIN32_FIND_DATAA entry;

    HANDLE h = FindFirstFileExA(
            path.c_str(),
            FindExInfoBasic, // Don't need the alternate name
            &entry, // Find data
            FindExSearchNameMatch, // Do not filter
            NULL, // Search filter. Always NULL.
            FIND_FIRST_EX_LARGE_FETCH // Try to increase performance
            );

    if (h == INVALID_HANDLE_VALUE)
        return entries;

    do {
        if (isDotOrDotDot(entry.cFileName)) continue;

        entries.push_back(DirEntry {
                entry->cFileName,
                entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY
                });

    } while (FindFileA(h, &entry));

    FindClose(h);

#else // _WIN32

    DIR* dir = opendir(path.length() > 0 ? path.c_str() : ".");
    if (!dir) return entries; // TODO: Throw exception instead

    struct dirent* entry;
    struct stat statbuf;

    while ((entry = readdir(dir))) {
        if (isDotOrDotDot(entry->d_name)) continue;

        if (entry->d_type == DT_UNKNOWN) {
            // Directory entry type is unknown. The file system is not required
            // to provide this information. Thus, we need to figure it out by
            // using stat.
            if (fstatat(dirfd(dir), entry->d_name, &statbuf, 0) == 0) {
                switch (statbuf.st_mode & S_IFMT) {
                    case S_IFIFO:  entry->d_type = DT_FIFO; break;
                    case S_IFCHR:  entry->d_type = DT_CHR;  break;
                    case S_IFDIR:  entry->d_type = DT_DIR;  break;
                    case S_IFBLK:  entry->d_type = DT_BLK;  break;
                    case S_IFREG:  entry->d_type = DT_REG;  break;
                    case S_IFLNK:  entry->d_type = DT_LNK;  break;
                    case S_IFSOCK: entry->d_type = DT_SOCK; break;
                }
            }
        }

        entries.push_back(DirEntry { entry->d_name, entry->d_type == DT_DIR });
    }

#endif // !_WIN32

    // Sort the entries. The order in which directories are listed is not
    // guaranteed to be deterministic.
    std::sort(entries.begin(), entries.end());

    return entries;
}

/**
 * Returns true if the given string contains a glob pattern.
 */
bool isGlobPattern(Path p) {
    for (size_t i = 0; i < p.length; ++i) {
        switch (p.path[i]) {
            case '?':
            case '*':
            case '[':
                return true;
        }
    }

    return false;
}

/**
 * Returns true if the given path element is a recursive glob pattern.
 */
bool isRecursiveGlob(Path p) {
    return p.length == 2 && p.path[0] == '*' && p.path[1] == '*';
}

struct GlobClosure {

    // Directory listing cache.
    DirCache* dirCache;

    // Root directory we are globbing from.
    Path root;

    Path pattern;

    // Next callback
    GlobCallback next;
    void* nextData;
};

void globCallback(Path path, bool isDir, void* data) {
    if (isDir) {
        const GlobClosure* c = (const GlobClosure*)data;
        c->dirCache->glob(c->root, path, c->pattern, c->next, c->nextData);
    }
}

}

/**
 * Helper function for listing a directory with the given pattern. If the
 * pattern is empty,
 */
void DirCache::glob(Path root, Path path, Path pattern,
          GlobCallback callback, void* data) {

    std::string buf(path.path, path.length);

    if (pattern.length == 0) {
        pattern.join(buf);
        callback(Path(buf.data(), buf.size()), true, data);
        return;
    }

    for (auto&& entry: dirEntries(root, path)) {
        if (globMatch(entry.name, pattern)) {
            Path(entry.name).join(buf);

            callback(Path(buf.data(), buf.size()), entry.isDir, data);

            buf.assign(path.path, path.length);
        }
    }
}

/**
 * Helper function to recursively yield directories for the given path.
 */
void DirCache::globRecursive(Path root, std::string& path,
        GlobCallback callback, void* data) {

    size_t len = path.size();

    // "**" matches 0 or more directories and thus includes this one.
    callback(Path(path.data(), path.size()), true, data);

    for (auto&& entry: dirEntries(root, path)) {
        Path(entry.name).join(path);

        callback(Path(path.data(), path.size()), entry.isDir, data);

        if (entry.isDir)
            globRecursive(root, path, callback, data);

        path.resize(len);
    }
}

/**
 * Glob a directory.
 */
void DirCache::glob(Path root, Path path, GlobCallback callback, void* data) {

    Split<Path> s = path.split();

    if (isGlobPattern(s.head)) {
        // Directory name contains a glob pattern

        GlobClosure c;
        c.dirCache = this;
        c.root = root;
        c.pattern = s.tail;
        c.next = callback;
        c.nextData = data;

        glob(root, s.head, &globCallback, &c);
    }
    else if (isRecursiveGlob(s.tail)) {
        std::string buf(s.head.path, s.head.length);
        globRecursive(root, buf, callback, data);
    }
    else if (isGlobPattern(s.tail)) {
        // Only base name contains a glob pattern.
        glob(root, s.head, s.tail, callback, data);
    }
    else {
        // No glob pattern in this path.
        if (s.tail.length) {
            // TODO: Only yield this path if the file exists.
            callback(path, false, data);
        }
        else {
            // TODO: Only yield this path if the directory exists.
            callback(s.head, true, data);
        }
    }
}

DirCache::DirCache(ImplicitDeps* deps) : _deps(deps) {}

const DirEntries& DirCache::dirEntries(Path root, Path dir) {
    std::string buf(root.path, root.length);
    dir.join(buf);
    return dirEntries(buf);
}

const DirEntries& DirCache::dirEntries(const std::string& path) {

    auto normalized = Path(path).norm();

    // Did we already do the work?
    const auto it = cache.find(normalized);
    if (it != cache.end())
        return it->second;

    if (_deps) _deps->addInput(normalized.data(), normalized.length());

    // List the directories, cache it, and return the cached list.
    return cache.insert(
            std::pair<std::string, DirEntries>(normalized, ::dirEntries(normalized))
            ).first->second;
}
