#include <dlfcn.h>

#include "hxroot.h"

static int (*access_real)(const char *path, int mode);
int access(const char *path, int mode) {
    if(!access_real) access_real = dlsym(RTLD_NEXT, "access");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    if(HxDebug) eprintf("access(\"%s\" -> \"%s\", %o)\n", path, new_path, mode);
    return access_real(new_path, mode);
}

static int (*faccessat_real)(int dirfd, const char *path, int mode, int flags);
int faccessat(int dirfd, const char *path, int mode, int flags) {
    if(!faccessat_real) faccessat_real = dlsym(RTLD_NEXT, "faccessat");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    if(HxDebug) eprintf("faccessat(%d, \"%s\" -> \"%s\", 0x%03o, 0x%x)\n", dirfd, path, new_path, mode, flags);
    return faccessat_real(dirfd, new_path, mode, flags);
}
