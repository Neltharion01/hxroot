#include <dlfcn.h>

#include "hxroot.h"

static int (*access_real)(const char *path, int mode);
int access(const char *path, int mode) {
    if(!access_real) access_real = dlsym(RTLD_NEXT, "access");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    if(HxDebug) eprintf("access(\"%s\" -> \"%s\", 0x%x)\n", path, new_path, mode);
    return access_real(new_path, mode);
}

static int (*faccessat_real)(int dirfd, const char *path, int mode, int flags);
int faccessat(int dirfd, const char *path, int mode, int flags) {
    if(!faccessat_real) faccessat_real = dlsym(RTLD_NEXT, "faccessat");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    if(HxDebug) eprintf("faccessat(%d, \"%s\" -> \"%s\", 0x%x, 0x%x)\n", dirfd, path, new_path, mode, flags);
    return faccessat_real(dirfd, new_path, mode, flags);
}

static int (*euidaccess_real)(const char *path, int mode);
int euidaccess(const char *path, int mode) {
    if(!euidaccess_real) euidaccess_real = dlsym(RTLD_NEXT, "euidaccess");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    if(HxDebug) eprintf("euidaccess(\"%s\" -> \"%s\", 0x%x)\n", path, new_path, mode);
    return euidaccess_real(new_path, mode);
}

int eaccess(const char *path, int mode) {
    return euidaccess(path, mode);
}
