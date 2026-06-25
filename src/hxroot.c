#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <dlfcn.h>

#include "hxroot.h"

STATIC char *HxRoot = 0;
STATIC size_t HxRootLen = 0;
STATIC char *HxBinds[16] = {0};
STATIC int HxBindsLen = -1;
STATIC bool HxDebug = false;
STATIC char *HxLinker = 0;
STATIC bool HxL2s = false;
STATIC char *HxProot = 0;
STATIC int HxUid = -1;
STATIC int HxGid = -1;

static void HxDoInit() {
    char *root = getenv("HxRoot");
    if(root) {
        HxRoot = root;
        HxRootLen = strlen(root);
    }

    char *binds = getenv("HxBinds");
    if(binds) {
        binds = strdup(binds);

        char *saveptr = NULL;
        char *tok = strtok_r(binds, " \n", &saveptr);
        int i = 0;
        while(tok != NULL) {
            HxBinds[i] = strdup(tok);
            tok = strtok_r(NULL, " \n", &saveptr);
            i += 1;
        }

        HxBindsLen = i;

        free(binds);
        binds = NULL;
    }

    char *dbg = getenv("HxDebug");
    if(dbg && atoi(dbg) == 1) HxDebug = true;

    char *linker = getenv("HxLinker");
    if(linker) HxLinker = strdup(linker);

    char *uid = getenv("HxUid");
    if(uid) HxUid = atoi(uid);

    char *gid = getenv("HxGid");
    if(gid) HxGid = atoi(gid);

    char *l2s = getenv("HxL2s");
    if(l2s && atoi(l2s)) HxL2s = true;

    char *proot = getenv("HxProot");
    if(proot) HxProot = strdup(proot);
}

static pthread_once_t HxOnce = PTHREAD_ONCE_INIT;

__attribute__((constructor))
STATIC void HxInit() {
    pthread_once(&HxOnce, HxDoInit);
}
