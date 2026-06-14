#include <stdlib.h>
#include <dlfcn.h>

#include "hxroot.h"

static int (*rename_real)(const char *old, const char *new);
int rename(const char *old, const char *new) {
    if(!rename_real) rename_real = dlsym(RTLD_NEXT, "rename");
    HxInit();

    int oldlen = HxL(old);
    char oldpathbuf[oldlen];
    const char *new_old = HxExpandPath(oldpathbuf, old);

    int newlen = HxL(new);
    char newpathbuf[newlen];
    const char *new_new = HxExpandPath(newpathbuf, new);

    if(HxDebug) eprintf("rename(\"%s\" -> \"%s\", \"%s\" -> \"%s\")\n", old, new_old, new, new_new);
    return rename_real(new_old, new_new);
}

int renameat(int oldfd, const char *old, int newfd, const char *new) {
    eprintf("UNIMPLEMENTED SHIT! renameat\n"); abort();
}
