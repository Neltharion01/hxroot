#include <stddef.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <stdlib.h>

#include "hxroot.h"

static ssize_t (*readlink_real)(const char *path, char *buf, size_t bufsize);
ssize_t readlink(const char *path, char *buf, size_t bufsize) {
    if(!readlink_real) readlink_real = dlsym(RTLD_NEXT, "readlink");
    HxInit();

    const char *new_path = path;
    if(HxRoot) new_path = HxExpandPath(path);
    if(HxDebug) eprintf("readlink(\"%s\" -> \"%s\", %p, %ld)\n", path, new_path, buf, bufsize);
    return readlink_real(new_path, buf, bufsize);
}
ssize_t readlinkat(int fd, const char *path, char *buf, size_t bufsize) {
    eprintf("UNIMPLEMENTED SHIT! readlinkat\n"); abort();
}
