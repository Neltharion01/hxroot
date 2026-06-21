#include <stddef.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

#include "hxroot.h"

static ssize_t (*readlink_real)(const char *path, char *buf, size_t bufsize);
ssize_t readlink(const char *path, char *buf, size_t bufsize) {
    if(!readlink_real) readlink_real = dlsym(RTLD_NEXT, "readlink");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    if(HxDebug) eprintf("readlink(\"%s\" -> \"%s\", %p, %ld)\n", path, new_path, buf, bufsize);

    ssize_t ret = readlink_real(new_path, buf, bufsize);
    if(ret == -1) return -1;
    if(ret == (ssize_t)bufsize) {
        // Truncated
        buf[bufsize-1] = '\0';
        // Still unexpand in case application uses truncated value
        HxUnexpandPath(buf);
        return ret;
    } else {
        buf[ret] = '\0';
        HxUnexpandPath(buf);
        return strlen(buf);
    }
}

ssize_t (*readlinkat_real)(int fd, const char *path, char *buf, size_t bufsize);
ssize_t readlinkat(int fd, const char *path, char *buf, size_t bufsize) {
    if(!readlinkat_real) readlinkat_real = dlsym(RTLD_NEXT, "readlinkat");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    ssize_t ret = readlinkat_real(fd, new_path, buf, bufsize-1);
    if(ret == -1) return -1;
    if(ret == (ssize_t)bufsize) {
        // Truncated
        buf[bufsize-1] = '\0';
        HxUnexpandPath(buf);
        return ret;
    } else {
        buf[ret] = '\0';
        HxUnexpandPath(buf);
        return strlen(buf);
    }
}
