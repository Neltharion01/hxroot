#include <unistd.h>

int chown(const char *path, uid_t owner, gid_t group) {
    return access(path, F_OK);
}

int fchown(int fd, uid_t owner, gid_t group) {
    return 0;
}

int lchown(const char *path, uid_t owner, gid_t group) {
    return access(path, F_OK);
}

int fchownat(int fd, const char *path, uid_t owner, gid_t group, int flag) {
    return faccessat(fd, path, F_OK, flag);
}
