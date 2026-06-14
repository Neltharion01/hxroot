#include <stdlib.h>
#include <dlfcn.h>

#include "hxroot.h"

static int (*rename_real)(const char *old, const char *new);
int rename(const char *old, const char *new) {
    if(!rename_real) rename_real = dlsym(RTLD_NEXT, "rename");
    HxInit();

    const char *new_old = old;
    const char *new_new = new;
    if(HxRoot) {
        new_old = HxExpandPath(old);
        new_new = HxExpandPath2(new);
    }

    if(HxDebug) eprintf("rename(\"%s\" -> \"%s\", \"%s\" -> \"%s\")\n", old, new_old, new, new_new);
    return rename_real(new_old, new_new);
}

int renameat(int oldfd, const char *old, int newfd, const char *new) {
    eprintf("UNIMPLEMENTED SHIT! renameat\n"); abort();
}
