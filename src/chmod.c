#include <dlfcn.h>
#include <sys/types.h>
#include <stdlib.h>

#include "hxroot.h"

static int (*chmod_real)(const char *path, mode_t mode);
int chmod(const char *path, mode_t mode) {
    if(!chmod_real) chmod_real = dlsym(RTLD_NEXT, "chmod");
    HxInit();

    const char *new_path = path;
    if(HxRoot) new_path = HxExpandPath(path);
    if(HxDebug) eprintf("chmod(\"%s\" -> \"%s\", %o)\n", path, new_path, mode);
    return chmod_real(new_path, mode);
}

int (*fchmodat_real)(int fd, const char *path, mode_t mode, int flag);
int fchmodat(int fd, const char *path, mode_t mode, int flag) {
    if(!fchmodat_real) fchmodat_real = dlsym(RTLD_NEXT, "fchmodat");
    HxInit();

    const char *new_path = path;
    if(HxRoot) new_path = HxExpandPath(path);
    if(HxDebug) eprintf("fchmodat(%d, \"%s\" -> \"%s\", %o, 0x%x)\n", fd, path, new_path, mode, flag);
    return fchmodat_real(fd, new_path, mode, flag);
}
