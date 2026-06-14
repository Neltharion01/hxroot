#define _GNU_SOURCE
#include <dlfcn.h>

#include "hxroot.h"

static void *(*dlopen_real)(const char *path, int flags);
void *dlopen(const char *path, int flags) {
    if(!dlopen_real) dlopen_real = dlsym(RTLD_NEXT, "dlopen");
    HxInit();

    const char *new_path = path;
    if(HxRoot) new_path = HxExpandPath(path);
    if(HxDebug) eprintf("dlopen(\"%s\" -> \"%s\", 0x%x)\n", path, new_path, flags);
    return dlopen_real(new_path, flags);
}

static void *(*dlmopen_real)(Lmid_t lmid, const char *path, int flags);
void *dlmopen(Lmid_t lmid, const char *path, int flags) {
    if(!dlmopen_real) dlmopen_real = dlsym(RTLD_NEXT, "dlmopen");
    HxInit();

    const char *new_path = path;
    if(HxRoot) new_path = HxExpandPath(path);
    if(HxDebug) eprintf("dlmopen(%d, \"%s\" -> \"%s\", 0x%x)\n", lmid, path, new_path, flags);
    return dlmopen_real(lmid, new_path, flags);
}
