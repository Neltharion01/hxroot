#include <sys/statvfs.h>
#include <dlfcn.h>

#include "hxroot.h"

int (*statvfs_real)(const char *restrict path, struct statvfs *restrict buf);
int statvfs(const char *restrict path, struct statvfs *restrict buf) {
    if(!statvfs_real) statvfs_real = dlsym(RTLD_NEXT, "statvfs");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    return statvfs_real(new_path, buf);
}
