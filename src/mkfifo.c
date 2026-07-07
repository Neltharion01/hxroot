#include <sys/types.h>
#include <dlfcn.h>

#include "hxroot.h"

static int (*mkfifo_real)(const char *path, mode_t mode);
int mkfifo(const char *path, mode_t mode) {
    if(!mkfifo_real) mkfifo_real = dlsym(RTLD_NEXT, "mkfifo");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    if(HxDebug) eprintf("mkfifo(\"%s\" -> \"%s\", %o)\n", path, new_path, mode);
    return mkfifo_real(new_path, mode);
}

static int (*mkfifoat_real)(int dirfd, const char *path, mode_t mode);
int mkfifoat(int dirfd, const char *path, mode_t mode) {
    if(!mkfifoat_real) mkfifoat_real = dlsym(RTLD_NEXT, "mkfifoat");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    if(HxDebug) eprintf("mkfifoat(%d, \"%s\" -> \"%s\", %o)\n", dirfd, path, new_path, mode);
    return mkfifoat_real(dirfd, new_path, mode);
}
