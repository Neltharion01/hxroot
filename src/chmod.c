#include <dlfcn.h>
#include <sys/types.h>
#include <stdlib.h>

#include "hxroot.h"

static int (*chmod_real)(const char *path, mode_t mode);
int chmod(const char *path, mode_t mode) {
    if(!chmod_real) chmod_real = dlsym(RTLD_NEXT, "chmod");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    if(HxDebug) eprintf("chmod(\"%s\" -> \"%s\", %o)\n", path, new_path, mode);
    return chmod_real(new_path, mode);
}

static int (*lchmod_real)(const char *path, mode_t mode);
int lchmod(const char *path, mode_t mode) {
    if(!lchmod_real) lchmod_real = dlsym(RTLD_NEXT, "lchmod");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    if(HxDebug) eprintf("lchmod(\"%s\" -> \"%s\", %o)\n", path, new_path, mode);
    return lchmod_real(new_path, mode);
}

static int (*fchmodat_real)(int fd, const char *path, mode_t mode, int flag);
int fchmodat(int fd, const char *path, mode_t mode, int flag) {
    if(!fchmodat_real) fchmodat_real = dlsym(RTLD_NEXT, "fchmodat");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    if(HxDebug) eprintf("fchmodat(%d, \"%s\" -> \"%s\", %o, 0x%x)\n", fd, path, new_path, mode, flag);
    return fchmodat_real(fd, new_path, mode, flag);
}
