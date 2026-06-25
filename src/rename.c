#define _GNU_SOURCE
#include <stdlib.h>
#include <dlfcn.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>

#include "hxroot.h"

static int (*rename_real)(const char *old, const char *new);
int rename(const char *old, const char *new) {
    if(!rename_real) rename_real = dlsym(RTLD_NEXT, "rename");
    HxInit();

    if(HxL2s) {
        struct stat st = {0};
        int ret = stat(new, &st);
        if(ret == -1 && errno != ENOENT) return -1;
        if(ret == 0 && st.st_mode & S_IFLNK) {
            // Unlink destination because it could be a "hardlink"
            // Regular symlink would have been overwritten anyway
            ret = unlink(new);
            if(ret == -1 && errno != ENOENT) return -1;
        }
    }

    int oldlen = HxL(old);
    char oldpathbuf[oldlen];
    const char *new_old = HxExpandPath(oldpathbuf, old);

    int newlen = HxL(new);
    char newpathbuf[newlen];
    const char *new_new = HxExpandPath(newpathbuf, new);

    if(HxDebug) eprintf("rename(\"%s\" -> \"%s\", \"%s\" -> \"%s\")\n", old, new_old, new, new_new);
    return rename_real(new_old, new_new);
}

int (*renameat_real)(int oldfd, const char *old, int newfd, const char *new);
int renameat(int oldfd, const char *old, int newfd, const char *new) {
    if(!renameat_real) renameat_real = dlsym(RTLD_NEXT, "renameat");
    HxInit();

    if(HxL2s) {
        struct stat st = {0};
        int ret = stat(new, &st);
        if(ret == -1 && errno != ENOENT) return -1;
        if(ret == 0 && st.st_mode & S_IFLNK) {
            // Unlink destination because it could be a "hardlink"
            // Regular symlink would have been overwritten anyway
            ret = unlink(new);
            if(ret == -1 && errno != ENOENT) return -1;
        }
    }

    int oldlen = HxL(old);
    char oldpathbuf[oldlen];
    const char *new_old = HxExpandPath(oldpathbuf, old);

    int newlen = HxL(new);
    char newpathbuf[newlen];
    const char *new_new = HxExpandPath(newpathbuf, new);

    if(HxDebug) eprintf("renameat(%d, \"%s\" -> \"%s\", %d, \"%s\" -> \"%s\")\n", oldfd, old, new_old, newfd, new, new_new);
    return renameat_real(oldfd, new_old, newfd, new_new);
}

int (*renameat2_real)(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, unsigned int flags);
int renameat2(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, unsigned int flags) {
    if(!renameat2_real) renameat2_real = dlsym(RTLD_NEXT, "renameat2");
    HxInit();

    if(HxL2s && !(flags & RENAME_NOREPLACE)) {
        struct stat st = {0};
        int ret = stat(newpath, &st);
        if(ret == -1 && errno != ENOENT) return -1;
        if(ret == 0 && st.st_mode & S_IFLNK) {
            // Unlink destination because it could be a "hardlink"
            // Regular symlink would have been overwritten anyway
            ret = unlink(newpath);
            if(ret == -1 && errno != ENOENT) return -1;
        }
    }

    int oldlen = HxL(oldpath);
    char oldpathbuf[oldlen];
    const char *new_oldpath = HxExpandPath(oldpathbuf, oldpath);

    int newlen = HxL(newpath);
    char newpathbuf[newlen];
    const char *new_newpath = HxExpandPath(newpathbuf, newpath);

    if(HxDebug) eprintf("renameat2(%d, \"%s\" -> \"%s\", %d, \"%s\" -> \"%s\", %d)\n", olddirfd, oldpath, new_oldpath, newdirfd, newpath, new_newpath, flags);
    return renameat2_real(olddirfd, new_oldpath, newdirfd, new_newpath, flags);
}
