#include <stddef.h>

void eprintf(char *fmt, ...);

size_t HxExpandedLen(const char *path);
#define HxL(path) HxExpandedLen(path)
const char *HxExpandPath(char *dest, const char *path);
void HxUnexpandPath(char *path);

typedef int HxFlock_t;
HxFlock_t HxFlock(int fd);

void HxAutoCloseFd(int *fd);
void HxAutoFreeChar(char **ptr);
void HxAutoUnlock(HxFlock_t *fl);
