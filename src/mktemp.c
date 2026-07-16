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
int mkstemp64(char *template) __attribute__((alias("mkstemp")));

static int (*mkostemp_real)(char *template, int flags);
int mkostemp(char *template, int flags) {
    if(!mkostemp_real) mkostemp_real = dlsym(RTLD_NEXT, "mkostemp");
    HxInit();

    int len = HxL(template);
    char pathbuf[len];
    char *new_template = (char*)HxExpandPath(pathbuf, template);

    if(HxDebug) eprintf("mkostemp(\"%s\" -> \"%s\", %d)\n", template, new_template, flags);

    int ret = mkostemp_real(new_template, flags);
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

static int (*mkstemps_real)(char *template, int suffixlen);
int mkstemps(char *template, int suffixlen) {
    if(!mkstemps_real) mkstemps_real = dlsym(RTLD_NEXT, "mkstemps");
    HxInit();

    int len = HxL(template);
    char pathbuf[len];
    char *new_template = (char*)HxExpandPath(pathbuf, template);

    if(HxDebug) eprintf("mkstemps(\"%s\" -> \"%s\", %d)\n", template, new_template, suffixlen);

    int ret = mkstemps_real(new_template, suffixlen);
    if(ret != -1 && len != 0) {
        template = strnul(template) - suffixlen - 6;
        new_template = strnul(new_template) - suffixlen - 6;
        template[0] = new_template[0];
        template[1] = new_template[1];
        template[2] = new_template[2];
        template[3] = new_template[3];
        template[4] = new_template[4];
        template[5] = new_template[5];
    }
    return ret;
}

int mkostemps(char *template, int suffixlen, int flags) {
    eprintf("UNIMPLEMENTED SHIT! mkostemps\n"); abort();
}
int mkostemps64(char *template, int suffixlen, int flags) __attribute__((alias("mkostemps")));

static char *(*mkdtemp_real)(char *template);
char *mkdtemp(char *template) {
    if(!mkdtemp_real) mkdtemp_real = dlsym(RTLD_NEXT, "mkdtemp");
    HxInit();

    int len = HxL(template);
    char pathbuf[len];
    char *new_template = (char*)HxExpandPath(pathbuf, template);

    if(HxDebug) eprintf("mkdtemp(\"%s\" -> \"%s\")\n", template, new_template);

    char *ret = mkdtemp_real(new_template);
    if(ret && len != 0) {
        ret = template;

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
