#include <wordexp.h>
#include <dlfcn.h>

#include "hxroot.h"

int (*wordexp_real)(const char *restrict s, wordexp_t *restrict p, int flags);
int wordexp(const char *restrict s, wordexp_t *restrict p, int flags) {
    if(!wordexp_real) wordexp_real = dlsym(RTLD_NEXT, "wordexp");
    HxInit();

    int len = HxL(s);
    char pathbuf[len];
    const char *new_s = HxExpandPath(pathbuf, s);

    int ret = wordexp_real(new_s, p, flags);
    if(ret == 0) {
        for(char **cur = p->we_wordv; *cur; cur++) {
            HxUnexpandPath(*cur);
        }
    }

    return ret;
}
