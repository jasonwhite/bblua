/**
 * Copyright (c) Jason White
 *
 * MIT License
 */

#ifdef _WIN32
#   include <windows.h>
#   include <codecvt>
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
#ifdef _WIN32

bool isDotOrDotDot(const wchar_t* p) {
    return (*p++ == L'.' && (*p == L'\0' || (*p++ == L'.' && *p == L'\0')));
}
#else

bool isDotOrDotDot(const char* p) {
    return (*p++ == '.' && (*p == '\0' || (*p++ == '.' && *p == '\0')));
}

#endif

/**
 * Returns a list of the files in a directory.
 */
DirEntries dirEntries(const std::string& path) {

    DirEntries entries;

#ifdef _WIN32

    // Convert path to UTF-16
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring widePath = converter.from_bytes(path);

    // We need "*.*" at the end of the path to list everything (even file names
    // that don't have a dot in them). Oh the insanity!
    widePath.append(L"\\*.*");

    WIN32_FIND_DATAW entry;

    HANDLE h = FindFirstFileExW(
            widePath.c_str(),
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
                converter.to_bytes(entry.cFileName),
                (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    == FILE_ATTRIBUTE_DIRECTORY
                });

    } while (FindNextFileW(h, &entry));

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
};

}

/**
 * Helper function for listing a directory with the given pattern. If the
 * pattern is empty,
 */
void DirCache::glob(Path root, Path path, Path pattern,
          GlobCallback callback) {

    std::string buf(path.path, path.length);

    if (pattern.length == 0) {
        pattern.join(buf);
        callback(Path(buf), true);
        return;
    }

    for (auto&& entry: dirEntries(root, path)) {
        if (globMatch(entry.name, pattern)) {
            Path(entry.name).join(buf);

            callback(Path(buf), entry.isDir);

            buf.resize(path.length);
        }
    }
}

/**
 * Helper function to recursively yield directories for the given path.
 */
void DirCache::globRecursive(Path root, std::string& path,
        GlobCallback callback) {

    size_t len = path.size();

    // "**" matches 0 or more directories and thus includes this one.
    callback(Path(path), true);

    for (auto&& entry: dirEntries(root, path)) {
        Path(entry.name).join(path);

        callback(Path(path), entry.isDir);

        if (entry.isDir) {
            //globRecursive(root, path, callback);
            _pool.enqueueTask([this, root, path, callback] {
                std::string copy(path);
                this->globRecursive(root, copy, callback);
                });
        }

        path.resize(len);
    }
}

/**
 * Glob a directory.
 */
void DirCache::glob(Path root, Path path, GlobCallback callback) {

    Split<Path> s = path.split();

    if (isGlobPattern(s.head)) {
        // Directory name contains a glob pattern.
        glob(root, s.head,
            [&](Path p, bool isDir) {
                if (isDir) this->glob(root, p, s.tail, callback);
            }
        );
    }
    else if (isRecursiveGlob(s.tail)) {
        std::string buf(s.head.path, s.head.length);
        globRecursive(root, buf, callback);
    }
    else if (isGlobPattern(s.tail)) {
        // Only base name contains a glob pattern.
        glob(root, s.head, s.tail, callback);
    }
    else {
        // No glob pattern in this path.
        if (s.tail.length) {
            callback(path, false);
        }
        else {
            callback(s.head, true);
        }
    }

    _pool.waitAll();
}

DirCache::DirCache(ImplicitDeps* deps) : _deps(deps), _pool(8) {}

DirCache::~DirCache() {
    // Ensure all work is done.
    _pool.waitAll();
}

const DirEntries& DirCache::dirEntries(Path root, Path dir) {
    std::string buf(root.path, root.length);
    dir.join(buf);
    return dirEntries(buf);
}

const DirEntries& DirCache::dirEntries(const std::string& path) {

    auto normalized = Path(path).norm();

    std::lock_guard<std::mutex> lock(_mutex);

    // Did we already do the work?
    const auto it = _cache.find(normalized);
    if (it != _cache.end())
        return it->second;

    if (_deps) _deps->addInput(normalized.data(), normalized.length());

    // List the directories, cache it, and return the cached list.
    return _cache.insert(
            std::pair<std::string, DirEntries>(normalized, ::dirEntries(normalized))
            ).first->second;
}
