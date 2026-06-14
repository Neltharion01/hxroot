#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <linux/limits.h>
#include <unistd.h>

#include "hxroot.h"

STATIC void eprintf(char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    vfprintf(stderr, fmt, args);

    va_end(args);
}

STATIC void HxExpandPath_r(char *dest, const char *path) {
    // Not absolute = return
    if(path[0] != '/') { strncpy(dest, path, PATH_MAX-1); return; }
    // Check excludes...
    for(char **cur = HxBinds; *cur != 0; cur++) {
        size_t prefix_len = strlen(*cur);
        // Compare by prefix
        if(strncmp(path, *cur, prefix_len) == 0) {
            strncpy(dest, path, PATH_MAX-1);
            return;
        }
    }
    // Already expanded = return
    if(strncmp(path, HxRoot, HxRootLen) == 0) {
        strncpy(dest, path, PATH_MAX-1);
        return;
    }

    snprintf(dest, PATH_MAX-1, "%s%s", HxRoot, path);
}

static _Thread_local char HxPathBuf[PATH_MAX];
STATIC char *HxExpandPath(const char *path) {
    HxExpandPath_r(HxPathBuf, path);
    return HxPathBuf;
}

static _Thread_local char HxPathBuf2[PATH_MAX];
STATIC char *HxExpandPath2(const char *path) {
    HxExpandPath_r(HxPathBuf2, path);
    return HxPathBuf2;
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
