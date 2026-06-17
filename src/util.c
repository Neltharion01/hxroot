#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "hxroot.h"

STATIC void eprintf(char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    vfprintf(stderr, fmt, args);

    va_end(args);
}

static bool HxShouldExpand(const char *path) {
    // Not chrooted?
    if(!HxRoot) return false;
    // Not absolute?
    if(path[0] != '/') return false;
    // Check excludes...
    for(int i = 0; HxBinds[i] != 0; i++) {
        size_t prefix_len = strlen(HxBinds[i]);
        // Compare by prefix
        if(strncmp(path, HxBinds[i], prefix_len) == 0) {
            return false;
        }
    }

    return true;
}

STATIC size_t HxExpandedLen(const char *path) {
    if(!HxShouldExpand(path)) {
        return 0;
    } else {
        return HxRootLen + strlen(path) + 1;
    }
}

STATIC const char *HxExpandPath(char *dest, const char *path) {
    if(!HxShouldExpand(path)) return path;
    char *next = stpcpy(dest, HxRoot);
    strcpy(next, path);
    return dest;
}

STATIC void HxUnexpandPath(char *path) {
    // If prefix equals to root...
    if(strncmp(path, HxRoot, HxRootLen) == 0) {
        // ...move everything after it to beginning
        strcpy(path, path + HxRootLen);
        // And if it is empty, add a slash
        if(path[0] == '\0') strcpy(path, "/");
    }
}

STATIC void HxAutoCloseFd(int *fd) {
    if(*fd != -1) {
        close(*fd);
        *fd = -1;
    }
}
