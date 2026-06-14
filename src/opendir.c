#include <dirent.h>
#include <dlfcn.h>

#include "hxroot.h"

static DIR *(*opendir_real)(const char *name);
DIR *opendir(const char *name) {
    if(!opendir_real) opendir_real = dlsym(RTLD_NEXT, "opendir");
    HxInit();

    const char *new_name = name;
    if(HxRoot) new_name = HxExpandPath(name);
    if(HxDebug) eprintf("opendir(\"%s\" -> \"%s\")\n", name, new_name);
    return opendir_real(new_name);
}
