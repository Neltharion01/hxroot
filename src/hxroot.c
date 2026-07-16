#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <dlfcn.h>

#include "hxroot.h"

PRIVATE char *HxRoot = 0;
PRIVATE size_t HxRootLen = 0;
PRIVATE char *HxBinds[16] = {0};
PRIVATE int HxBindsLen = -1;
PRIVATE bool HxDebug = false;
PRIVATE char *HxLinker = 0;
PRIVATE bool HxL2s = false;
PRIVATE char *HxProot = 0;
PRIVATE int HxUid = -1;
PRIVATE int HxGid = -1;

PRIVATE char **HxEnviron = 0;

extern char **environ;

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

    size_t size = HxCountArgv(environ)+1;
    HxEnviron = calloc(size, sizeof(char*));
    memcpy(HxEnviron, environ, size*sizeof(char*));
}

static pthread_once_t HxOnce = PTHREAD_ONCE_INIT;

__attribute__((constructor))
PRIVATE void HxInit() {
    pthread_once(&HxOnce, HxDoInit);
}
