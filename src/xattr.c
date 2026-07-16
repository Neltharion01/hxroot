#include <sys/types.h>
#include <dlfcn.h>

#include "hxroot.h"

static ssize_t (*listxattr_real)(const char *path, char *_Nullable list, size_t size);
ssize_t listxattr(const char *path, char *_Nullable list, size_t size) {
    if(!listxattr_real) listxattr_real = dlsym(RTLD_NEXT, "listxattr");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    return listxattr_real(new_path, list, size);
}

static ssize_t (*llistxattr_real)(const char *path, char *_Nullable list, size_t size);
ssize_t llistxattr(const char *path, char *_Nullable list, size_t size) {
    if(!llistxattr_real) llistxattr_real = dlsym(RTLD_NEXT, "llistxattr");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    return llistxattr_real(new_path, list, size);
}

static ssize_t (*getxattr_real)(const char *path, const char *name, void *value, size_t size);
ssize_t getxattr(const char *path, const char *name, void *value, size_t size) {
    if(!getxattr_real) getxattr_real = dlsym(RTLD_NEXT, "getxattr");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    return getxattr_real(new_path, name, value, size);
}

static ssize_t (*lgetxattr_real)(const char *path, const char *name, void *value, size_t size);
ssize_t lgetxattr(const char *path, const char *name, void *value, size_t size) {
    if(!lgetxattr_real) lgetxattr_real = dlsym(RTLD_NEXT, "lgetxattr");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    return lgetxattr_real(new_path, name, value, size);
}

static int (*setxattr_real)(const char *path, const char *name, const void *value, size_t size, int flags);
int setxattr(const char *path, const char *name, const void *value, size_t size, int flags) {
    if(!setxattr_real) setxattr_real = dlsym(RTLD_NEXT, "setxattr");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    return setxattr_real(new_path, name, value, size, flags);
}

static int (*lsetxattr_real)(const char *path, const char *name, const void *value, size_t size, int flags);
int lsetxattr(const char *path, const char *name, const void *value, size_t size, int flags) {
    if(!lsetxattr_real) lsetxattr_real = dlsym(RTLD_NEXT, "lsetxattr");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    return lsetxattr_real(new_path, name, value, size, flags);
}

static int (*removexattr_real)(const char *path, const char *name);
int removexattr(const char *path, const char *name) {
    if(!removexattr_real) removexattr_real = dlsym(RTLD_NEXT, "removexattr");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    return removexattr_real(new_path, name);
}

static int (*lremovexattr_real)(const char *path, const char *name);
int lremovexattr(const char *path, const char *name) {
    if(!lremovexattr_real) lremovexattr_real = dlsym(RTLD_NEXT, "lremovexattr");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    return lremovexattr_real(new_path, name);
}
