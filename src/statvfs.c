#include <sys/vfs.h>
#include <sys/statvfs.h>
#include <dlfcn.h>
#include <stdlib.h>

#include "hxroot.h"

static int (*statfs_real)(const char *path, struct statfs *buf);
int statfs(const char *path, struct statfs *buf) {
    if(!statfs_real) statfs_real = dlsym(RTLD_NEXT, "statfs");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    return statfs_real(new_path, buf);
}

static int (*statvfs_real)(const char *restrict path, struct statvfs *restrict buf);
int statvfs(const char *restrict path, struct statvfs *restrict buf) {
    if(!statvfs_real) statvfs_real = dlsym(RTLD_NEXT, "statvfs");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    return statvfs_real(new_path, buf);
}
