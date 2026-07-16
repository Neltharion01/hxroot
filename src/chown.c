#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>

#include "hxroot.h"

static int (*chown_real)(const char *path, uid_t owner, gid_t group);
int chown(const char *path, uid_t owner, gid_t group) {
    if(!chown_real) chown_real = dlsym(RTLD_NEXT, "chown");
    HxInit();

    if(HxUid == 0 && HxGid == 0) return access(path, F_OK);

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    return chown_real(new_path, owner, group);
}

static int (*fchown_real)(int fd, uid_t owner, gid_t group);
int fchown(int fd, uid_t owner, gid_t group) {
    if(!fchown_real) fchown_real = dlsym(RTLD_NEXT, "fchown");
    HxInit();

    if(HxUid == 0 && HxGid == 0) return 0;

    return fchown_real(fd, owner, group);
}

static int (*lchown_real)(const char *path, uid_t owner, gid_t group);
int lchown(const char *path, uid_t owner, gid_t group) {
    if(!lchown_real) lchown_real = dlsym(RTLD_NEXT, "lchown");
    HxInit();

    if(HxUid == 0 && HxGid == 0) return faccessat(AT_FDCWD, path, F_OK, AT_SYMLINK_NOFOLLOW);

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    return lchown_real(new_path, owner, group);
}

static int (*fchownat_real)(int fd, const char *path, uid_t owner, gid_t group, int flag);
int fchownat(int fd, const char *path, uid_t owner, gid_t group, int flag) {
    if(!fchownat_real) fchownat_real = dlsym(RTLD_NEXT, "fchownat");
    HxInit();

    if(HxUid == 0 && HxGid == 0) return faccessat(fd, path, F_OK, flag);

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    return fchownat_real(fd, new_path, owner, group, flag);
}
