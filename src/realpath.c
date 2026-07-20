#include <dlfcn.h>

#include "hxroot.h"

static char *(*realpath_real)(const char *path, char *resolved_path);
char *realpath(const char *path, char *resolved_path) {
    if(!realpath_real) realpath_real = dlsym(RTLD_NEXT, "realpath");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    if(HxDebug) eprintf("realpath(\"%s\" -> \"%s\")\n", path, new_path);

    char *ret = realpath_real(new_path, resolved_path);
    if(!ret) return ret;

    HxUnexpandPath(ret);
    return ret;
}

char *canonicalize_file_name(const char *path) {
    return realpath(path, NULL);
}
