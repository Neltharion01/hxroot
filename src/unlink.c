#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>

#include "hxroot.h"

int remove(const char *path) {
    eprintf("UNIMPLEMENTED SHIT! remove\n"); abort();
}

int (*rmdir_real)(const char *path);
int rmdir(const char *path) {
    if(!rmdir_real) rmdir_real = dlsym(RTLD_NEXT, "rmdir");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    return rmdir_real(new_path);
}

static int (*unlink_real)(const char *path);
static int (*unlinkat_real)(int fd, const char *path, int flag);

static int HxL2sUnlink(const char *link, const char *linkfile, int realfd, const char *realfile) {
    if(!unlink_real) unlink_real = dlsym(RTLD_NEXT, "unlink");
    if(!unlinkat_real) unlinkat_real = dlsym(RTLD_NEXT, "unlinkat");

    AUTO_CLOSE int fd = open(linkfile, O_RDWR);
    if(fd == -1) return -1;

    AUTO_UNLOCK HxFlock_t l = HxFlock(fd);
    if(l == -1) return -1;

    int linkcount;
    int nr = read(fd, &linkcount, sizeof(int));
    if(nr == -1) {
        return -1;
    } else if(nr != sizeof(int)) {
        errno = ENODATA;
        return -1;
    }
    linkcount -= 1;

    if(linkcount == 0) {
        int linklen = HxL(link);
        char linkbuf[linklen];
        const char *new_link = HxExpandPath(linkbuf, link);
        if(unlink_real(new_link) == -1) return -1;

        int linkfilelen = HxL(linkfile);
        char linkfilebuf[linkfilelen];
        const char *new_linkfile = HxExpandPath(linkfilebuf, linkfile);
        if(unlink_real(new_linkfile) == -1) return -1;
    } else {
        if(lseek(fd, 0, SEEK_SET) == -1) return -1;

        int nw = write(fd, &linkcount, sizeof(int));
        if(nw == -1) {
            return -1;
        } else if(nw != sizeof(int)) {
            errno = EPIPE;
            return -1;
        }
    }

    int realpathlen = HxL(realfile);
    char realpathbuf[realpathlen];
    const char *new_realfile = HxExpandPath(realpathbuf, realfile);
    if(unlinkat_real(realfd, new_realfile, 0) == -1) return -1;

    return 0;
}

int unlink(const char *path) {
    if(!unlink_real) unlink_real = dlsym(RTLD_NEXT, "unlink");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    if(HxL2s) {
        char rlbuf[PATH_MAX];
        int ret = readlink(new_path, rlbuf, PATH_MAX-1);
        if(ret != -1) {
            rlbuf[ret] = '\0';
            AUTO_FREE_CHAR char *linkpath = NULL;
            if(asprintf(&linkpath, "%s.links", rlbuf) == -1) return -1;
            if(access(linkpath, F_OK) == 0) {
                // Unlink
                return HxL2sUnlink(rlbuf, linkpath, AT_FDCWD, path);
            } else if(errno == ENOENT) {
                // Not a link
            } else {
                // Error
                return -1;
            }
        } else if(errno != EINVAL) {
            return -1;
        }
    }

    if(HxDebug) eprintf("unlink(\"%s\" -> \"%s\")\n", path, new_path);
    return unlink_real(new_path);
}

int unlinkat(int fd, const char *path, int flag) {
    if(!unlinkat_real) unlinkat_real = dlsym(RTLD_NEXT, "unlinkat");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    if(HxL2s) {
        char rlbuf[PATH_MAX];
        int ret = readlinkat(fd, new_path, rlbuf, PATH_MAX-1);
        if(ret != -1) {
            rlbuf[ret] = '\0';
            AUTO_FREE_CHAR char *linkpath = NULL;
            if(asprintf(&linkpath, "%s.links", rlbuf) == -1) return -1;
            if(access(linkpath, F_OK) == 0) {
                // Unlink
                return HxL2sUnlink(rlbuf, linkpath, fd, path);
            } else if(errno == ENOENT) {
                // Not a link
            } else {
                // Error
                return -1;
            }
        } else if(errno != EINVAL) {
            return -1;
        }
    }

    if(HxDebug) eprintf("unlinkat(%d, \"%s\" -> \"%s\", 0x%x)\n", fd, path, new_path, flag);
    return unlinkat_real(fd, new_path, flag);
}
