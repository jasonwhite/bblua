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

enum class PathType {
    // The path doesn't exist.
    missing,

    // The path type is unknown.
    unknown,

    // The path exists refers to a file.
    file,

    // The path exists and refers to a directory.
    dir,
};

/**
 * Returns the type of a given path. That is, if it exists, if it's a directory,
 * or if it's a file.
 */
PathType pathType(const char* path) {
    struct stat statbuf;

    if (lstat(path, &statbuf) != 0)
        return PathType::unknown;

    switch (statbuf.st_mode & S_IFMT) {
        case S_IFREG: return PathType::file;
        case S_IFDIR: return PathType::dir;
    }

    return PathType::unknown;
}

PathType pathType(const Path root, const Path path) {
    std::string buf(root.path, root.length);
    path.join(buf);
    return pathType(buf.c_str());
}

/**
 * Returns true if the given string contains a glob pattern.
 */
inline bool isGlobPattern(const Path& p) {
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
inline bool isRecursiveGlob(const Path& p) {
    return p.length == 2 && p.path[0] == '*' && p.path[1] == '*';
}

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

void DirCache::glob(Path root, Path path, MatchCallback callback) {

    bool onlyMatchDirs = path.basename().length == 0;

    auto components = path.components();

    std::string buf;

    globImpl(root, buf, components, 0, onlyMatchDirs, callback);
}

void DirCache::globImpl(Path root, std::string& path,
        const std::vector<Path>& components, size_t index,
        bool matchDirs, MatchCallback callback) {

    if (index >= components.size()) return;

    const Path& pattern = components[index];

    // We only want to use the callback if this is the last thing to match.
    const bool lastOne = index == components.size()-1;

    const size_t pathLength = path.size();

    if (!isGlobPattern(pattern)) {
        Path(pattern).join(path);

        if (lastOne) {
            // The explicitly named path must exist in order to be returned.
            PathType type = pathType(root, path);
            if (( matchDirs && type == PathType::dir) ||
                (!matchDirs && type == PathType::file)) {
                callback(path);
            }
        }
        else {
            // Assume its a directory and go deeper
            globImpl(root, path, components, index+1, matchDirs, callback);
        }

        path.resize(pathLength);
    }
    else if (isRecursiveGlob(pattern)) {
        // A recursive glob can match 0 or more directories. Lets assume here it
        // will match 0 directories. Note that this will cause the same
        // directory to be listed twice. This should be okay since we are
        // caching directory listing results.
        globImpl(root, path, components, index+1, matchDirs, callback);

        // We also want to continue on here attempting to match more than 0
        // directories.
        for (auto&& entry: dirEntries(root, path)) {

            Path(entry.name).join(path);

            if (lastOne && entry.isDir == matchDirs) {
                // Note that "**" matches all files recursively and "**/"
                // matches all directories recursively. Thus, we yield this
                // path if this is the last pattern in the list and we've found
                // the type of entry we're looking for.
                callback(path);
            }

            if (entry.isDir) {
                // We can match 0 or more directories. Go deeper!
                globImpl(root, path, components, index, matchDirs, callback);
            }

            path.resize(pathLength);
        }
    }
    else {
        for (auto&& entry: dirEntries(root, path)) {
            if (!globMatch(entry.name, pattern)) continue;

            Path(entry.name).join(path);

            if (lastOne) {
                if (entry.isDir == matchDirs)
                    callback(path);
            }
            else if (entry.isDir) {
                // It's a directory and it matched. Shift the pattern.
                globImpl(root, path, components, index+1, matchDirs, callback);
            }

            path.resize(pathLength);
        }
    }
}
