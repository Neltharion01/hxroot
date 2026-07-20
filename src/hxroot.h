#include <stddef.h>
#include <stdbool.h>

#define EXPORT __attribute__((visibility("default")))
#define PRIVATE __attribute__((visibility("hidden")))
#define AUTO_CLOSE __attribute__((cleanup(HxAutoCloseFd)))
#define AUTO_FREE_CHAR __attribute__((cleanup(HxAutoFreeChar)))
#define AUTO_UNLOCK __attribute__((cleanup(HxAutoUnlock)))
#define AUTO_FREE_ELFINFO __attribute__((cleanup(HxDestroyElfInfo)))
#define AUTO_FREE_LDLIB __attribute__((cleanup(HxDestroyLdlib)))

#define _Nullable

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

// Environ backup to workaround programs which modify environ directly (hello shadow)
extern char **HxEnviron;
// Faked /proc/self/exe path
extern char *HxExe;
extern int HxExeLen;

#include "util.h"

void HxInit();
