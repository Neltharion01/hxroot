#include <mntent.h>
#include <dlfcn.h>

#include "util.h"

static FILE *(*setmntent_real)(const char *path, const char *type);
FILE *setmntent(const char *path, const char *type) {
    if(!setmntent_real) setmntent_real = dlsym(RTLD_NEXT, "setmntent");

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    return setmntent_real(new_path, type);
}
