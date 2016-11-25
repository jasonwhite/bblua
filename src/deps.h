/**
 * Copyright (c) Jason White
 *
 * MIT License
 *
 * Description:
 * Handles sending dependencies to parent build system.
 */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#ifdef _WIN32
#   pragma warning(push)

    // Disable "nonstandard extension used: zero-sized array in struct/union"
#   pragma warning(disable : 4200)
#endif

#pragma pack(push, 4)

struct Dependency {

    /**
     * Status of the resource.
     *
     * Can be:
     *  0: Status is unknown.
     *  1: Resource does not exist.
     *  2: The resource is a file.
     *  3: The resource is a directory.
     */
    uint32_t status;

    /**
     * SHA-256 checksum of the contents of the resource. If unknown or not
     * computed, this should be set to 0. In such a case, the parent build
     * system will compute the value when needed.
     *
     * For files, this is the checksum of the file contents. For directories,
     * this is the checksum of the paths in the sorted directory listing.
     */
    uint8_t checksum[32];

    /**
     * Length of the name.
     */
    uint32_t length;

    /**
     * Name of the resource that can be used to lookup the data. Length is given
     * by the length member.
     *
     * This is usually a file or directory path. The path do not need to be
     * normalized. If a relative path, the build system assumes it is relative
     * to the working directory that the child was spawned in.
     */
    char name[0];
};

#pragma pack(pop)

/**
 * Handles sending dependencies to the parent build system (if any).
 *
 * When creating child processes, the parent build system will set the
 * environment variables BUTTON_INPUTS and BUTTON_OUTPUTS to the file descriptor
 * that can be used to send back dependency information from the child process.
 * This is the generic interface for making implicit inputs and outputs known to
 * the parent build system.
 */
class ImplicitDeps {
private:

#ifdef _WIN32
    void* _inputs;
    void* _outputs;
#else
    FILE* _inputs;
    FILE* _outputs;
#endif

public:
    ImplicitDeps();
    ~ImplicitDeps();

    /**
     * Returns true if there is a parent build system to send dependencies to.
     */
    bool hasParent() const;

    /**
     * Adds the given dependency.
     */
    void addInput(const Dependency& dep);
    void addOutput(const Dependency& dep);

    /**
     * Adds a dependency by name only.
     */
    void addInput(const char* name, size_t length);
    void addOutput(const char* name, size_t length);
};


#ifdef _WIN32
#   pragma warning(pop)
#endif
