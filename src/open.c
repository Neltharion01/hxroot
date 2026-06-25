#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <dlfcn.h>

#include "hxroot.h"

static int (*open_real)(const char *path, int flags, ...);
int open(const char *path, int flags, ...) {
    if(!open_real) open_real = dlsym(RTLD_NEXT, "open");
    HxInit();

    mode_t mode = 0;

    va_list args;
    va_start(args, flags);

    if (flags & O_CREAT) {
        mode = va_arg(args, mode_t);
    }

    va_end(args);

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    if(HxDebug) eprintf("open(\"%s\" -> \"%s\", 0x%x, 0o%o)\n", path, new_path, flags, mode);
    return open_real(new_path, flags, mode);
}
int open64(const char *path, int flags, ...) __attribute__((alias("open")));

static int (*openat_real)(int dirfd, const char *path, int flags, ...);
int openat(int dirfd, const char *path, int flags, ...) {
    if(!openat_real) openat_real = dlsym(RTLD_NEXT, "openat");
    HxInit();

    mode_t mode = 0;

    va_list args;
    va_start(args, flags);

    if (flags & O_CREAT) {
        mode = va_arg(args, mode_t);
    }

    va_end(args);

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    if(HxDebug) eprintf("openat(%d, \"%s\" -> \"%s\", 0x%x, 0o%o)\n", dirfd, path, new_path, flags, mode);
    return openat_real(dirfd, new_path, flags, mode);
}

static FILE *(*fopen_real)(const char *path, const char *mode);
FILE *fopen(const char *path, const char *mode) {
    if(!fopen_real) fopen_real = dlsym(RTLD_NEXT, "fopen");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    if(HxDebug) eprintf("fopen(\"%s\" -> \"%s\", \"%s\")\n", path, new_path, mode);
    return fopen_real(new_path, mode);
}
FILE *fopen64(const char *path, const char *mode) __attribute__((alias("fopen")));

FILE *(*freopen_real)(const char *restrict path, const char *restrict mode, FILE *restrict stream);
FILE *freopen(const char *restrict path, const char *restrict mode, FILE *restrict stream) {
    if(!freopen_real) freopen_real = dlsym(RTLD_NEXT, "freopen");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    if(HxDebug) eprintf("freopen(\"%s\" -> \"%s\", \"%s\", %p)\n", path, new_path, mode, stream);
    return freopen_real(new_path, mode, stream);
}
FILE *freopen64(const char *restrict path, const char *restrict mode, FILE *restrict stream) __attribute__((alias("freopen")));
