#include <stdlib.h>
#include <dlfcn.h>

#include "hxroot.h"

static int (*mkdir_real)(const char *path, mode_t mode);
int mkdir(const char *path, mode_t mode) {
    if(!mkdir_real) mkdir_real = dlsym(RTLD_NEXT, "mkdir");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    if(HxDebug) eprintf("mkdir(\"%s\" -> \"%s\", %03o)\n", path, new_path, mode);
    return mkdir_real(new_path, mode);
}

int (*mkdirat_real)(int fd, const char *path, mode_t mode);
int mkdirat(int fd, const char *path, mode_t mode) {
    if(!mkdirat_real) mkdirat_real = dlsym(RTLD_NEXT, "mkdirat");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    if(HxDebug) eprintf("mkdirat(%d, \"%s\" -> \"%s\", %03o)\n", fd, path, new_path, mode);
    return mkdirat_real(fd, new_path, mode);
}
