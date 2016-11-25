/**
 * Copyright (c) Jason White
 *
 * MIT License
 *
 * Description:
 * Handles sending dependencies to parent build system.
 */
#include "deps.h"

#include <stdlib.h>

#ifdef _WIN32

#include <windows.h>

ImplicitDeps::ImplicitDeps() : _inputs(NULL), _outputs(NULL) {

    static const size_t bufLength = 32;
    char buf[bufLength];

    size_t len = 0;

    len = GetEnvironmentVariableA("BUTTON_INPUTS", buf, bufLength);
    if (len == 0 || len >= bufLength)
        return;

    _inputs = (void*)strtoull(buf, NULL, 10);

    len = GetEnvironmentVariableA("BUTTON_OUTPUTS", buf, bufLength);
    if (len == 0 || len >= bufLength)
        return;

    _outputs = (void*)strtoull(buf, NULL, 10);
}

ImplicitDeps::~ImplicitDeps() {
    if (_inputs)  CloseHandle(_inputs);
    if (_outputs) CloseHandle(_outputs);
}

bool ImplicitDeps::hasParent() const {
    return _inputs != NULL || _outputs != NULL;
}

void ImplicitDeps::addInput(const Dependency& dep) {
    if (!_inputs) return;

    DWORD written;
    WriteFile(_inputs, &dep, sizeof(dep) + dep.length, &written, NULL);
}

void ImplicitDeps::addOutput(const Dependency& dep) {
    if (!_outputs) return;

    DWORD written;
    WriteFile(_outputs, &dep, sizeof(dep) + dep.length, &written, NULL);
}

void ImplicitDeps::addInput(const char* name, size_t length) {
    if (!_inputs) return;

    if (length > UINT32_MAX)
        length = UINT32_MAX;

    Dependency dep = {0};
    dep.length = (uint32_t)length;

    DWORD written;
    WriteFile(_inputs, &dep, sizeof(dep), &written, NULL);
    WriteFile(_inputs, name, dep.length, &written, NULL);
}

void ImplicitDeps::addOutput(const char* name, size_t length) {
    if (!_outputs) return;

    if (length > UINT32_MAX)
        length = UINT32_MAX;

    Dependency dep = {0};
    dep.length = (uint32_t)length;

    DWORD written;
    WriteFile(_outputs, &dep, sizeof(dep), &written, NULL);
    WriteFile(_outputs, name, dep.length, &written, NULL);
}

#else // WIN32

ImplicitDeps::ImplicitDeps() : _inputs(NULL), _outputs(NULL) {
    const char* var;
    int fd;

    var = getenv("BUTTON_INPUTS");
    if (var && (fd = atoi(var)))
        _inputs = fdopen(fd, "a");

    var = getenv("BUTTON_OUTPUTS");
    if (var && (fd = atoi(var)))
        _outputs = fdopen(fd, "a");
}

ImplicitDeps::~ImplicitDeps() {
    if (_inputs)  fclose(_inputs);
    if (_outputs) fclose(_outputs);
}

bool ImplicitDeps::hasParent() const {
    return _inputs != NULL || _outputs != NULL;
}

void ImplicitDeps::addInput(const Dependency& dep) {
    if (!_inputs) return;

    fwrite(&dep, sizeof(dep) + dep.length, 1, _inputs);
}

void ImplicitDeps::addOutput(const Dependency& dep) {
    if (!_outputs) return;

    fwrite(&dep, sizeof(dep) + dep.length, 1, _outputs);
}

void ImplicitDeps::addInput(const char* name, size_t length) {
    if (!_inputs) return;

    if (length > UINT32_MAX)
        length = UINT32_MAX;

    Dependency dep = {0};
    dep.length = (uint32_t)length;

    fwrite(&dep, sizeof(dep), 1, _inputs);
    fwrite(name, 1, dep.length, _inputs);
}

void ImplicitDeps::addOutput(const char* name, size_t length) {
    if (!_outputs) return;

    if (length > UINT32_MAX)
        length = UINT32_MAX;

    Dependency dep = {0};
    dep.length = (uint32_t)length;

    fwrite(&dep, sizeof(dep), 1, _outputs);
    fwrite(name, 1, dep.length, _outputs);
}
#endif // !_WIN32
