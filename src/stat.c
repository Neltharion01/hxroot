#define _GNU_SOURCE
#include <sys/stat.h>
#include <dlfcn.h>

#include "hxroot.h"

static int (*stat_real)(const char *path, struct stat *statbuf);
int stat(const char *path, struct stat *statbuf) {
    if(!stat_real) stat_real = dlsym(RTLD_NEXT, "stat");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    if(HxDebug) eprintf("stat(\"%s\" -> \"%s\", %p)\n", path, new_path, statbuf);

    int ret = stat_real(new_path, statbuf);
    if(ret == 0) {
        if(HxUid != -1) statbuf->st_uid = HxUid;
        if(HxGid != -1) statbuf->st_gid = HxGid;
    }
    return ret;
}
int stat64(const char *path, struct stat64 *statbuf) __attribute__((alias("stat")));

static int (*fstat_real)(int fd, struct stat *statbuf);
int fstat(int fd, struct stat *statbuf) {
    if(!fstat_real) fstat_real = dlsym(RTLD_NEXT, "fstat");
    HxInit();

    if(HxDebug) eprintf("fstat(%d, %p)\n", fd, statbuf);

    int ret = fstat_real(fd, statbuf);
    if(ret == 0) {
        if(HxUid != -1) statbuf->st_uid = HxUid;
        if(HxGid != -1) statbuf->st_gid = HxGid;
    }
    return ret;
}

static int (*lstat_real)(const char *path, struct stat *statbuf);
int lstat(const char *path, struct stat *statbuf) {
    if(!lstat_real) lstat_real = dlsym(RTLD_NEXT, "lstat");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    if(HxDebug) eprintf("lstat(\"%s\" -> \"%s\", %p)\n", path, new_path, statbuf);

    int ret = lstat_real(new_path, statbuf);
    if(ret == 0) {
        if(HxUid != -1) statbuf->st_uid = HxUid;
        if(HxGid != -1) statbuf->st_gid = HxGid;
    }
    return ret;
}
int lstat64(const char *path, struct stat64 *statbuf) __attribute__((alias("lstat")));

static int (*fstatat_real)(int dirfd, const char *path, struct stat *statbuf, int flags);
int fstatat(int dirfd, const char *path, struct stat *statbuf, int flags) {
    if(!fstatat_real) fstatat_real = dlsym(RTLD_NEXT, "fstatat");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    if(HxDebug) eprintf("fstatat(%d, \"%s\" -> \"%s\", %p, 0x%x)\n", dirfd, path, new_path, statbuf, flags);

    int ret = fstatat_real(dirfd, new_path, statbuf, flags);
    if(ret == 0) {
        if(HxUid != -1) statbuf->st_uid = HxUid;
        if(HxGid != -1) statbuf->st_gid = HxGid;
    }
    return ret;
}
int fstatat64(int dirfd, const char *path, struct stat64 *statbuf, int flags) __attribute__((alias("fstatat")));

static int (*statx_real)(int dirfd, const char *path, int flags, unsigned int mask, struct statx *statxbuf);
int statx(int dirfd, const char *path, int flags, unsigned int mask, struct statx *statxbuf) {
    if(!statx_real) statx_real = dlsym(RTLD_NEXT, "statx");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    if(HxDebug) eprintf("statx(%d, \"%s\" -> \"%s\", %x, %x, %p)\n", dirfd, path, new_path, flags, mask, statxbuf);

    int ret = statx_real(dirfd, new_path, flags, mask, statxbuf);
    if(ret == 0) {
        if(HxUid != -1 && mask & STATX_UID) statxbuf->stx_uid = HxUid;
        if(HxGid != -1 && mask & STATX_GID) statxbuf->stx_gid = HxGid;
    }
    return ret;
}
