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

static int (*link_real)(const char *path1, const char *path2);
int link(const char *path1, const char *path2) {
    if(!link_real) link_real = dlsym(RTLD_NEXT, "link");
    HxInit();

    if(HxL2s) return symlink(path1, path2);

    int len1 = HxL(path1);
    char pathbuf1[len1];
    const char *new_path1 = HxExpandPath(pathbuf1, path1);

    int len2 = HxL(path2);
    char pathbuf2[len2];
    const char *new_path2 = HxExpandPath(pathbuf2, path2);

    if(HxDebug) eprintf("link(\"%s\" -> \"%s\", \"%s\" -> \"%s\")\n", path1, new_path1, path2, new_path2);

    return link_real(new_path1, new_path2);
}

int linkat(int fd1, const char *path1, int fd2, const char *path2, int flag) {
    eprintf("UNIMPLEMENTED SHIT! linkat\n"); abort();
}
