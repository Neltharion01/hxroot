#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/random.h>

#include "hxroot.h"

static int (*symlink_real)(const char *target, const char *linkpath);
int symlink(const char *target, const char *linkpath) {
    if(!symlink_real) symlink_real = dlsym(RTLD_NEXT, "symlink");
    HxInit();

    int len = HxL(linkpath);
    char pathbuf[len];
    const char *new_linkpath = HxExpandPath(pathbuf, linkpath);

    int targetlen = HxL(target);
    char targetbuf[targetlen];
    const char *new_target = HxExpandPath(targetbuf, target);

    if(HxDebug) eprintf("symlink(\"%s\", \"%s\" -> \"%s\")\n", target, linkpath, new_linkpath);
    return symlink_real(new_target, new_linkpath);
}

static int (*symlinkat_real)(const char *target, int newdirfd, const char *linkpath);
int symlinkat(const char *target, int newdirfd, const char *linkpath) {
    if(!symlinkat_real) symlinkat_real = dlsym(RTLD_NEXT, "symlinkat");
    HxInit();

    int len = HxL(linkpath);
    char pathbuf[len];
    const char *new_linkpath = HxExpandPath(pathbuf, linkpath);

    int targetlen = HxL(target);
    char targetbuf[targetlen];
    const char *new_target = HxExpandPath(targetbuf, target);

    if(HxDebug) eprintf("symlinkat(\"%s\", %d, \"%s\" -> \"%s\")\n", target, newdirfd, linkpath, new_linkpath);
    return symlinkat_real(new_target, newdirfd, new_linkpath);
}

char *HxL2sName(const char *path) {
    char *pathbuf = strdupa(path);

    char *lastslash = strrchr(pathbuf, '/');
    char *dirname, *basename;
    if(!lastslash) {
        dirname = "\0";
        basename = pathbuf;
    } else {
        *lastslash = '\0';
        dirname = pathbuf;
        basename = lastslash+1;
    }

    char *ret = NULL;
    if(asprintf(&ret, "%s/.HxL2s.%s.XXXXXX", dirname, basename) == -1) return NULL;
    return ret;
}

int HxL2sCreate(const char *linkcountfile) {
    AUTO_CLOSE int fd = open(linkcountfile, O_CREAT|O_RDWR|O_EXCL, 0o777);
    if(fd == -1) return -1;
    // Lock the file
    AUTO_UNLOCK HxFlock_t lock = HxFlock(fd);
    if(lock == -1) return -1;
    // New file + old file
    int linkcount = 2;
    int nw = write(fd, &linkcount, sizeof(int));
    if(nw == -1) return -1;
    if(nw != sizeof(int)) {
        errno = EPIPE;
        return -1;
    }
    // File will unlock automatically
    return 0;
}

int HxL2sIncrement(const char *linkcountfile) {
    AUTO_CLOSE int fd = open(linkcountfile, O_RDWR, 0o777);
    if(fd == -1) return -1;
    // Lock the file
    AUTO_UNLOCK HxFlock_t lock = HxFlock(fd);
    if(lock == -1) return -1;
    // Try reading the count...
    int linkcount;
    int nr = read(fd, &linkcount, sizeof(int));
    if(nr == -1) return -1;
    if(nr != sizeof(int)) {
        errno = ENODATA;
        return -1;
    }

    linkcount += 1;

    if(lseek(fd, 0, SEEK_SET) == -1) return -1;

    int nw = write(fd, &linkcount, sizeof(int));
    if(nw == -1) return -1;
    if(nw != sizeof(int)) {
        errno = EPIPE;
        return -1;
    }

    // File will unlock automatically
    return 0;
}

static const char *MktempChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
static const int MktempCharsLen = sizeof(MktempChars)-1;
static int HxMktemp(char *tmpl) {
    char r[6];
    ssize_t ret = getrandom(r, 6, 0);
    if(ret == -1) {
        return -1;
    } else if(ret != 6) {
        errno = ENODATA;
        return -1;
    }

    char *XXXXXX = tmpl + strlen(tmpl) - 6;
    if(strcmp(XXXXXX, "XXXXXX") != 0) {
        errno = EINVAL;
        return -1;
    }

    XXXXXX[0] = MktempChars[r[0] % MktempCharsLen];
    XXXXXX[1] = MktempChars[r[1] % MktempCharsLen];
    XXXXXX[2] = MktempChars[r[2] % MktempCharsLen];
    XXXXXX[3] = MktempChars[r[3] % MktempCharsLen];
    XXXXXX[4] = MktempChars[r[4] % MktempCharsLen];
    XXXXXX[5] = MktempChars[r[5] % MktempCharsLen];

    return 0;
}

// oldpath - existing file
// newpath - link to be created
static int HxLink2symlink(const char *oldpath, const char *newpath) {
    // If newpath exists, return EEXIST
    if(access(newpath, F_OK) == 0) {
        errno = EEXIST;
        return -1;
    } else if(errno != ENOENT) {
        return -1;
    }

    // Resolve absolute oldpath
    AUTO_FREE_CHAR char *rp = realpath(oldpath, NULL);
    if(!rp) return -1;

    AUTO_FREE_CHAR char *links = NULL;
    if(asprintf(&links, "%s.hxlinks", rp) == -1) return -1;
    if(access(links, F_OK) == 0) {
        // It is a hardlink
        if(HxL2sIncrement(links) == -1) return -1;
        if(symlink(rp, newpath) == -1) return -1;
        return 0;
    } else if(errno == ENOENT) {
        // It is not a link
        AUTO_FREE_CHAR char *hardlink = HxL2sName(rp);
        if(HxMktemp(hardlink) == -1) return -1;
        AUTO_FREE_CHAR char *hardlink_links = NULL;
        if(asprintf(&hardlink_links, "%s.hxlinks", hardlink) == -1) return -1;

        if(HxL2sCreate(hardlink_links) == -1) return -1;
        if(rename(oldpath, hardlink) == -1) return -1;
        if(symlink(hardlink, oldpath) == -1) return -1;
        if(symlink(hardlink, newpath) == -1) return -1;
        return 0;
    } else {
        // Error occured
        return -1;
    }
}

static int (*link_real)(const char *oldpath, const char *newpath);
int link(const char *oldpath, const char *newpath) {
    if(!link_real) link_real = dlsym(RTLD_NEXT, "link");
    HxInit();

    if(HxL2s) return HxLink2symlink(oldpath, newpath);

    int len1 = HxL(oldpath);
    char pathbuf1[len1];
    const char *new_oldpath = HxExpandPath(pathbuf1, oldpath);

    int len2 = HxL(newpath);
    char pathbuf2[len2];
    const char *new_newpath = HxExpandPath(pathbuf2, newpath);

    if(HxDebug) eprintf("link(\"%s\" -> \"%s\", \"%s\" -> \"%s\")\n", oldpath, new_oldpath, newpath, new_newpath);

    return link_real(new_oldpath, new_newpath);
}

int (*linkat_real)(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flag);
int linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flag) {
    if(!linkat_real) linkat_real = dlsym(RTLD_NEXT, "linkat");
    HxInit();

    if(HxL2s) {
        if(olddirfd != AT_FDCWD || newdirfd != AT_FDCWD) {
            eprintf("linkat is not fully implemented\n"); abort();
        }
        if(flag != 0) {
            eprintf("linkat flags are not supported\n"); abort();
        }
        return HxLink2symlink(oldpath, newpath);
    }

    int len1 = HxL(oldpath);
    char pathbuf1[len1];
    const char *new_oldpath = HxExpandPath(pathbuf1, oldpath);

    int len2 = HxL(newpath);
    char pathbuf2[len2];
    const char *new_newpath = HxExpandPath(pathbuf2, newpath);

    return linkat_real(olddirfd, new_oldpath, newdirfd, new_newpath, flag);
}
