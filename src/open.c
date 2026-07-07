#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <errno.h>
#include <sys/stat.h>

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

    int ret = open_real(new_path, flags, mode);

    if(ret == -1 && errno == EACCES && HxUid == 0 && HxGid == 0) {
        // Simulate CAP_DAC_OVERRIDE
        struct stat st = {0};
        if(stat(path, &st) == -1) return -1;

        int new_mode = st.st_mode;
        if(flags & O_WRONLY) {
            new_mode |= S_IWUSR;
        } else if(flags & O_RDWR) {
            new_mode |= S_IRUSR|S_IWUSR;
        } else {
            new_mode |= S_IRUSR;
        }

        if(chmod(path, new_mode) == -1) return -1;

        ret = open_real(new_path, flags, mode);

        if(chmod(path, st.st_mode) == -1) return -1;
    }

    return ret;
}
int open64(const char *path, int flags, ...) __attribute__((alias("open")));

static int (*creat_real)(const char *path, mode_t mode);
int creat(const char *path, mode_t mode) {
    if(!creat_real) creat_real = dlsym(RTLD_NEXT, "creat");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    return creat_real(new_path, mode);
}

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

    int ret = openat_real(dirfd, new_path, flags, mode);

    if(ret == -1 && errno == EACCES && HxUid == 0 && HxGid == 0) {
        // Simulate CAP_DAC_OVERRIDE
        struct stat st = {0};
        if(stat(path, &st) == -1) return -1;

        int new_mode = st.st_mode;
        if(flags & O_WRONLY) {
            new_mode |= S_IWUSR;
        } else if(flags & O_RDWR) {
            new_mode |= S_IRUSR|S_IWUSR;
        } else {
            new_mode |= S_IRUSR;
        }

        if(fchmodat(dirfd, path, new_mode, 0) == -1) return -1;

        ret = openat_real(dirfd, new_path, flags, mode);

        if(chmod(path, st.st_mode) == -1) return -1;
    }

    return ret;
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

static FILE *(*freopen_real)(const char *restrict path, const char *restrict mode, FILE *restrict stream);
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

int __open_2(const char *path, int oflag) {
    return open(path, oflag);
}
