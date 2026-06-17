#include <stdlib.h>
#include <dlfcn.h>

#include "hxroot.h"

static int (*symlink_real)(const char *target, const char *linkpath);
int symlink(const char *target, const char *linkpath) {
    if(!symlink_real) symlink_real = dlsym(RTLD_NEXT, "symlink");
    HxInit();

    int len = HxL(linkpath);
    char pathbuf[len];
    const char *new_linkpath = HxExpandPath(pathbuf, linkpath);

    if(HxDebug) eprintf("symlink(\"%s\", \"%s\" -> \"%s\")\n", target, linkpath, new_linkpath);
    return symlink_real(target, new_linkpath);
}

int symlinkat(const char *target, int newdirfd, const char *linkpath) {
    eprintf("UNIMPLEMENTED SHIT! symlinkat\n"); abort();
}

static int (*link_real)(const char *oldpath, const char *newpath);
int link(const char *oldpath, const char *newpath) {
    if(!link_real) link_real = dlsym(RTLD_NEXT, "link");
    HxInit();

    if(HxL2s) return symlink(oldpath, newpath);

    int len1 = HxL(oldpath);
    char pathbuf1[len1];
    const char *new_oldpath = HxExpandPath(pathbuf1, oldpath);

    int len2 = HxL(newpath);
    char pathbuf2[len2];
    const char *new_newpath = HxExpandPath(pathbuf2, newpath);

    if(HxDebug) eprintf("link(\"%s\" -> \"%s\", \"%s\" -> \"%s\")\n", oldpath, new_oldpath, newpath, new_newpath);

    return link_real(new_oldpath, new_newpath);
}

int linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flag) {
    eprintf("UNIMPLEMENTED SHIT! linkat\n"); abort();
}
