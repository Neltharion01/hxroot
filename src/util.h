#include <stddef.h>

// printf to stderr
void eprintf(char *fmt, ...);

// Returns length for expanded version of this path, or 0 if `HxExpandPath` will not need a buffer
size_t HxExpandedLen(const char *path);
// Also available under `HxL` alias
#define HxL(path) HxExpandedLen(path)

// Expands path, prepending HxRoot contents to it. Example usage:
// size_t len = HxL(path);
// char pathbuf[len];
// char *new_path = HxExpandPath(pathbuf, path);
const char *HxExpandPath(char *dest, const char *path);

// Removes HxRoot part of path. Modifies string in-place
void HxUnexpandPath(char *path);

// Automatic destructor for file descriptors, used for `AUTO_CLOSE` macro
void HxAutoCloseFd(int *fd);
#define AUTO_CLOSE __attribute__((cleanup(HxAutoCloseFd)))
