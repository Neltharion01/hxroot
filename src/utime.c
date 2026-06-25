#include <stdlib.h>
#include <utime.h>
#include <dlfcn.h>

#include "hxroot.h"

int utime(const char *path, const struct utimbuf *times) {
    eprintf("UNIMPLEMENTED SHIT! utime\n"); abort();
}

static int (*utimes_real)(const char *path, const struct timeval times[2]);
int utimes(const char *path, const struct timeval times[2]) {
    if(!utimes_real) utimes_real = dlsym(RTLD_NEXT, "utimes");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    if(HxDebug) eprintf("utimes(\"%s\" -> \"%s\", %p)\n", path, new_path, times);
    return utimes_real(new_path, times);
}

int (*lutimes_real)(const char *path, const struct timeval times[2]);
int lutimes(const char *path, const struct timeval times[2]) {
    if(!lutimes_real) lutimes_real = dlsym(RTLD_NEXT, "lutimes");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    if(HxDebug) eprintf("lutimes(\"%s\" -> \"%s\", %p)\n", path, new_path, times);
    return lutimes_real(new_path, times);
}

int (*utimensat_real)(int dirfd, const char *path, const struct timespec times[2], int flags);
int utimensat(int dirfd, const char *path, const struct timespec times[2], int flags) {
    if(!utimensat_real) utimensat_real = dlsym(RTLD_NEXT, "utimensat");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    if(HxDebug) eprintf("utimensat(%d, \"%s\" -> \"%s\", %p, 0x%x)\n", dirfd, path, new_path, times, flags);
    return utimensat_real(dirfd, new_path, times, flags);
}
