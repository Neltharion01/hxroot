#define _GNU_SOURCE
#include <sys/stat.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>

#include "hxroot.h"

static int (*fstatat_real)(int dirfd, const char *path, struct stat *statbuf, int flags);

static int HxReadLinkCount(int dirfd, const char *path, const char *new_path, struct stat *statbuf, struct statx *statxbuf) {
    if(!fstatat_real) fstatat_real = dlsym(RTLD_NEXT, "fstatat");

    struct stat st = {0};
    if(fstatat_real(dirfd, new_path, &st, AT_SYMLINK_NOFOLLOW) == -1) return -1;

    // Not a hardlink
    if(!S_ISLNK(st.st_mode)) return 0;

    if(st.st_size > 4096) {
        errno = E2BIG;
        return -1;
    }

    size_t lnkbufsiz = st.st_size+1;

    // Contents of proc have always their st.st_size=0
    if(lnkbufsiz-1 == 0) {
        lnkbufsiz = PATH_MAX;
    }

    char lnkbuf[lnkbufsiz];
    ssize_t ret = readlinkat(dirfd, path, lnkbuf, lnkbufsiz);
    if(ret == -1) return -1;
    lnkbuf[lnkbufsiz-1] = '\0';

    AUTO_FREE_CHAR char *rpbuf = NULL;
    if(lnkbuf[0] != '/') {
        int joinlen = strlen(path) + strlen(lnkbuf) + 2;
        char joined_lnkbuf[joinlen];
        strcpy(joined_lnkbuf, path);
        char *next = strrchr(joined_lnkbuf, '/');
        if(next) next += 1;
        else next = joined_lnkbuf;
        strcpy(next, lnkbuf);
        rpbuf = realpath(joined_lnkbuf, NULL);
    } else {
        rpbuf = realpath(lnkbuf, NULL);
    }

    // If couldn't resolve real path, just assume that link count is correct (for lstat)
    if(!rpbuf) return 0;

    AUTO_FREE_CHAR char *lnkcountfile = NULL;
    if(asprintf(&lnkcountfile, "%s.hxlinks", rpbuf) == -1) return -1;

    AUTO_CLOSE int fd = open(lnkcountfile, O_RDWR, 0o777);
    if(fd == -1 && errno == ENOENT) {
        // Was not a hardlink
        return 0;
    }
    if(fd == -1) return -1;

    int linkcount;
    int nr = read(fd, &linkcount, sizeof(int));
    if(nr == -1) return -1;
    if(nr != sizeof(int)) {
        errno = ENODATA;
        return -1;
    }

    if(statbuf) {
        statbuf->st_nlink = linkcount;
        statbuf->st_mode &= ~S_IFLNK;
        statbuf->st_mode |= S_IFREG;
    }
    if(statxbuf) {
        statxbuf->stx_nlink = linkcount;
        statxbuf->stx_mode &= ~S_IFLNK;
        statxbuf->stx_mode |= S_IFREG;
    }

    if(statbuf) statbuf->st_nlink = 2;

    return 0;
}

static int (*stat_real)(const char *path, struct stat *statbuf);
int stat(const char *path, struct stat *statbuf) {
    if(!stat_real) stat_real = dlsym(RTLD_NEXT, "stat");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    if(HxDebug) eprintf("stat(\"%s\" -> \"%s\", %p)\n", path, new_path, statbuf);

    int ret = stat_real(new_path, statbuf);
    if(ret == 0) {
        if(HxUid != -1) statbuf->st_uid = HxUid;
        if(HxGid != -1) statbuf->st_gid = HxGid;
        if(HxL2s) {
            if(HxReadLinkCount(AT_FDCWD, path, new_path, statbuf, NULL) == -1) return -1;
        }
    }
    return ret;
}
int stat64(const char *path, struct stat64 *statbuf) __attribute__((alias("stat")));

static int (*fstat_real)(int fd, struct stat *statbuf);
int fstat(int fd, struct stat *statbuf) {
    if(!fstat_real) fstat_real = dlsym(RTLD_NEXT, "fstat");
    HxInit();

    if(HxDebug) eprintf("fstat(%d, %p)\n", fd, statbuf);

    int ret = fstat_real(fd, statbuf);
    if(ret == 0) {
        if(HxUid != -1) statbuf->st_uid = HxUid;
        if(HxGid != -1) statbuf->st_gid = HxGid;
        // Not possible here
        /*if(HxL2s) {
            if(HxReadLinkCount(AT_FDCWD, path, new_path, statbuf, NULL) == -1) return -1;
        }*/
    }
    return ret;
}
int fstat64(int fd, struct stat64 *statbuf) __attribute__((alias("fstat")));

static int (*lstat_real)(const char *path, struct stat *statbuf);
int lstat(const char *path, struct stat *statbuf) {
    if(!lstat_real) lstat_real = dlsym(RTLD_NEXT, "lstat");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    if(HxDebug) eprintf("lstat(\"%s\" -> \"%s\", %p)\n", path, new_path, statbuf);

    int ret = lstat_real(new_path, statbuf);
    if(ret == 0) {
        if(HxUid != -1) statbuf->st_uid = HxUid;
        if(HxGid != -1) statbuf->st_gid = HxGid;
        if(HxL2s && S_ISLNK(statbuf->st_mode)) {
            // Technically if we land on symlink to hardlink then we shouldn't modify type and link count, but I don't know how to handle that imperfection, and it doesn't break anything
            if(HxReadLinkCount(AT_FDCWD, path, new_path, statbuf, NULL) == -1) return -1;
        }
    }
    return ret;
}
int lstat64(const char *path, struct stat64 *statbuf) __attribute__((alias("lstat")));

//static int (*fstatat_real)(int dirfd, const char *path, struct stat *statbuf, int flags);
int fstatat(int dirfd, const char *path, struct stat *statbuf, int flags) {
    if(!fstatat_real) fstatat_real = dlsym(RTLD_NEXT, "fstatat");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    if(HxDebug) eprintf("fstatat(%d, \"%s\" -> \"%s\", %p, 0x%x)\n", dirfd, path, new_path, statbuf, flags);

    int ret = fstatat_real(dirfd, new_path, statbuf, flags);
    if(ret == 0) {
        if(HxUid != -1) statbuf->st_uid = HxUid;
        if(HxGid != -1) statbuf->st_gid = HxGid;
        if(HxL2s) {
            if(HxReadLinkCount(dirfd, path, new_path, statbuf, NULL) == -1) return -1;
        }
    }
    return ret;
}
int fstatat64(int dirfd, const char *path, struct stat64 *statbuf, int flags) __attribute__((alias("fstatat")));

static int (*statx_real)(int dirfd, const char *path, int flags, unsigned int mask, struct statx *statxbuf);
int statx(int dirfd, const char *path, int flags, unsigned int mask, struct statx *statxbuf) {
    if(!statx_real) statx_real = dlsym(RTLD_NEXT, "statx");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    if(HxDebug) eprintf("statx(%d, \"%s\" -> \"%s\", %x, %x, %p)\n", dirfd, path, new_path, flags, mask, statxbuf);

    // Need to read type and size if we want real link count
    if(mask & STATX_NLINK) mask |= STATX_TYPE | STATX_SIZE;

    int ret = statx_real(dirfd, new_path, flags, mask, statxbuf);
    if(ret == 0) {
        if(HxUid != -1 && mask & STATX_UID) statxbuf->stx_uid = HxUid;
        if(HxGid != -1 && mask & STATX_GID) statxbuf->stx_gid = HxGid;
        if(HxL2s && (mask & STATX_NLINK) && (flags & AT_SYMLINK_NOFOLLOW) && S_ISLNK(statxbuf->stx_mode)) {
            if(HxReadLinkCount(dirfd, path, new_path, NULL, statxbuf) == -1) return -1;
        }
    }
    return ret;
}

int __xstat(int ver, const char *path, struct stat *statbuf) {
    return stat(path, statbuf);
}
int __xstat64(int ver, const char *path, struct stat *statbuf) __attribute__((alias("__xstat")));

int __fxstat(int ver, int fd, struct stat *statbuf) {
    return fstat(fd, statbuf);
}
int __fxstat64(int ver, int fd, struct stat *statbuf) __attribute__((alias("__fxstat")));

int __lxstat(int ver, const char *path, struct stat *statbuf) {
    return lstat(path, statbuf);
}
int __lxstat64(int ver, const char *path, struct stat *statbuf) __attribute__((alias("__lxstat")));
