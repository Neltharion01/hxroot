#include <stddef.h>
#include <grp.h>

// Temporary workaround for termux-glibc bug
struct group *getgrgid(gid_t gid) {
    return NULL;
}
