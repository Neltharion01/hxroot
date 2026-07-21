#include <dlfcn.h>

#include "hxroot.h"

static int (*attr_get_real)(const char *path, const char *attrname, char *attrvalue, int *valuelength, int flags);
int attr_get(const char *path, const char *attrname, char *attrvalue, int *valuelength, int flags) {
    if(!attr_get_real) attr_get_real = dlsym(RTLD_NEXT, "attr_get");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    return attr_get_real(new_path, attrname, attrvalue, valuelength, flags);
}

static int (*attr_set_real)(const char *path, const char *attrname, const char *attrvalue, const int valuelength, int flags);
int attr_set(const char *path, const char *attrname, const char *attrvalue, const int valuelength, int flags) {
    if(!attr_set_real) attr_set_real = dlsym(RTLD_NEXT, "attr_set");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    return attr_set_real(new_path, attrname, attrvalue, valuelength, flags);
}

static int (*attr_remove_real)(const char *path, const char *attrname, int flags);
int attr_remove(const char *path, const char *attrname, int flags) {
    if(!attr_remove_real) attr_remove_real = dlsym(RTLD_NEXT, "attr_remove");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    return attr_remove_real(new_path, attrname, flags);
}

static int (*attr_list_real)(const char *path, char *buffer, const int buffersize, int flags, void *cursor);
int attr_list(const char *path, char *buffer, const int buffersize, int flags, void *cursor) {
    if(!attr_list_real) attr_list_real = dlsym(RTLD_NEXT, "attr_list");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    return attr_list_real(new_path, buffer, buffersize, flags, cursor);
}

static int (*attr_multi_real)(const char *path, void *oplist, int count, int flags);
int attr_multi(const char *path, void *oplist, int count, int flags) {
    if(!attr_multi_real) attr_multi_real = dlsym(RTLD_NEXT, "attr_multi");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    return attr_multi_real(new_path, oplist, count, flags);
}
