/**
 * Copyright (c) Jason White
 *
 * MIT License
 *
 * Description:
 * A cache for the contents of a directory. This also handles reporting
 * directory dependencies.
 */
#pragma once

#include <map>
#include <string>
#include <vector>
#include <mutex>
#include <functional>

#include "threadpool.h"
#include "path.h"

class ImplicitDeps;

struct DirEntry {
    std::string name;
    bool isDir;

    // Implement ordering for sorting purposes.
    friend bool operator<(const DirEntry& a, const DirEntry& b);
};

typedef std::vector<DirEntry> DirEntries;

// Called with the matched path.
using MatchCallback = std::function<void(Path)>;

/**
 * A cache for directory listings.
 */
class DirCache {
private:
    // Mapping of directory names to directory contents.
    std::map<std::string, DirEntries> _cache;

    ImplicitDeps* _deps;

    // Mutex protects the cache.
    std::mutex _mutex;

    ThreadPool _pool;

public:
    DirCache(ImplicitDeps* deps = nullptr);
    virtual ~DirCache();

    /**
     * Returns a list of names in the given directory.
     */
    const DirEntries& dirEntries(const std::string& path);

    /**
     * Convenience function. The two paths are joined and then looked up.
     */
    const DirEntries& dirEntries(Path root, Path dir);

    /**
     * Globs for files starting at the given root.
     *
     * Parameters:
     *   root     = The root directory to start searching from. All matched
     *              paths are relative to this directory.
     *   path     = The path which can contain glob patterns. Recursive glob
     *              expressions are also supported.
     *   callback = The function to call for every matched file name.
     */
    void glob(Path root, Path path, MatchCallback callback);

private:

    void globImpl(
            Path root, // Root from which all matched paths are relative.
            std::string& path, // The directory path we've matched so far.
            const std::vector<Path>& components, // Path components of the pattern.
            size_t index, // Current component we're trying to match.
            bool matchDirs, // Only match directories.
            MatchCallback callback // Function to call for every match
            );
};
