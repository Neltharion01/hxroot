#include <sys/types.h>

int initgroups(const char *user, gid_t group) {
    return 0;
}

int setgroups(size_t size, const gid_t list[]) {
    return 0;
}
