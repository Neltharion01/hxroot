#include <stdlib.h>
#include <dlfcn.h>

#include "hxroot.h"

int remove(const char *path) {
    eprintf("UNIMPLEMENTED SHIT! remove\n"); abort();
}

int (*rmdir_real)(const char *path);
int rmdir(const char *path) {
    if(!rmdir_real) rmdir_real = dlsym(RTLD_NEXT, "rmdir");
    HxInit();
    const char *new_path = path;
    if(HxRoot) new_path = HxExpandPath(path);
    return rmdir_real(new_path);
}

static int (*unlink_real)(const char *path);
int unlink(const char *path) {
    if(!unlink_real) unlink_real = dlsym(RTLD_NEXT, "unlink");
    HxInit();

    const char *new_path = path;
    if(HxRoot) new_path = HxExpandPath(path);
    if(HxDebug) eprintf("unlink(\"%s\" -> \"%s\")\n", path, new_path);
    return unlink_real(new_path);
}

static int (*unlinkat_real)(int fd, const char *path, int flag);
int unlinkat(int fd, const char *path, int flag) {
    if(!unlinkat_real) unlinkat_real = dlsym(RTLD_NEXT, "unlinkat");
    HxInit();

    const char *new_path = path;
    if(HxRoot) new_path = HxExpandPath(path);
    if(HxDebug) eprintf("unlinkat(%d, \"%s\" -> \"%s\", 0x%x)\n", fd, path, new_path, flag);
    return unlinkat_real(fd, new_path, flag);
}
