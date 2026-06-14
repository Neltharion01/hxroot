#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

#include "hxroot.h"

static char *strnul(char* s) {
    return s + strlen(s);
}

static int (*mkstemp_real)(char *template);
int mkstemp(char *template) {
    if(!mkstemp_real) mkstemp_real = dlsym(RTLD_NEXT, "mkstemp");
    HxInit();

    int len = HxL(template);
    char pathbuf[len];
    char *new_template = (char*)HxExpandPath(pathbuf, template);

    if(HxDebug) eprintf("mkstemp(\"%s\" -> \"%s\")\n", template, new_template);

    int ret = mkstemp_real(new_template);
    if(ret != -1 && len != 0) {
        template = strnul(template) - 6;
        new_template = strnul(new_template) - 6;
        template[0] = new_template[0];
        template[1] = new_template[1];
        template[2] = new_template[2];
        template[3] = new_template[3];
        template[4] = new_template[4];
        template[5] = new_template[5];
    }
    return ret;
}

int mkostemp(char *template, int flags) {
    eprintf("UNIMPLEMENTED SHIT! mkostemp\n"); abort();
}
int mkstemps(char *template, int suffixlen) {
    eprintf("UNIMPLEMENTED SHIT! mkstemps\n"); abort();
}
int mkostemps(char *template, int suffixlen, int flags) {
    eprintf("UNIMPLEMENTED SHIT! mkostemps\n"); abort();
}
