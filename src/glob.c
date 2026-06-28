#include <glob.h>
#include <dlfcn.h>

#include "hxroot.h"

int (*glob_real)(const char *restrict pattern, int flags, typeof(int (const char *epath, int eerrno)) *errfunc, glob_t *restrict pglob);
int glob(const char *restrict pattern, int flags, typeof(int (const char *epath, int eerrno)) *errfunc, glob_t *restrict pglob) {
    if(!glob_real) glob_real = dlsym(RTLD_NEXT, "glob");
    HxInit();

    int len = HxL(pattern);
    char pathbuf[len];
    const char *new_pattern = HxExpandPath(pathbuf, pattern);

    int ret = glob_real(new_pattern, flags, errfunc, pglob);
    if(ret == 0) {
        for(char **cur = pglob->gl_pathv; *cur; cur++) {
            HxUnexpandPath(*cur);
        }
    }

    return ret;
}
