#include <stddef.h>
#include <dlfcn.h>

#include "hxroot.h"

static int (*chdir_real)(const char *path);
int chdir(const char *path) {
    if(!chdir_real) chdir_real = dlsym(RTLD_NEXT, "chdir");
    HxInit();

    const char *new_path = path;
    if(HxRoot) new_path = HxExpandPath(path);
    if(HxDebug) eprintf("chdir(\"%s\" -> \"%s\")\n", path, new_path);
    return chdir_real(new_path);
}

static char *(*getcwd_real)(char *buf, size_t size);
char *getcwd(char *buf, size_t size) {
    if(!getcwd_real) getcwd_real = dlsym(RTLD_NEXT, "getcwd");
    HxInit();

    char *ret = getcwd_real(buf, size);
    if(!ret) return ret;
    if(HxDebug) eprintf("getcwd() \"%s\" -> ", ret);
    HxUnexpandPath(ret);
    if(HxDebug) eprintf("\"%s\"\n", ret);
    return ret;
}
