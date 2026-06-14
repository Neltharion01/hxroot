#include <stddef.h>
#include <stdbool.h>

#define EXPORT __attribute__((visibility("default")))
#define STATIC __attribute__((visibility("hidden")))
#define AUTO_CLOSE __attribute__((cleanup(HxAutoCloseFd)))

extern char *HxRoot;
extern size_t HxRootLen;
extern char *HxBinds[16];
extern int HxBindsLen;
extern bool HxDebug;
extern char *HxLinker;
extern bool HxL2s;
extern char *HxProot;
extern int HxUid;
extern int HxGid;

#include "util.h"

void HxInit();
