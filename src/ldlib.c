#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hxroot.h"
#include "ldlib.h"

static void HxLdlibAllocate(HxLdlib *ldlib, size_t size) {
    if(!ldlib->buf) {
        ldlib->buf = malloc(size);
        ldlib->bufpos = 0;
        ldlib->bufsiz = size;
    }
}

PRIVATE void HxLdlib_set(HxLdlib *ldlib, char *s) {
    size_t slen = strlen(s) + 1;
    HxLdlibAllocate(ldlib, slen>512 ? slen : 512);

    strcpy(ldlib->buf, s);
    ldlib->bufpos = strlen(s);
}

PRIVATE void HxLdlib_append(HxLdlib *ldlib, char *s) {
    size_t slen = strlen("LD_LIBRARY_PATH=") + strlen(s) + 1;
    HxLdlibAllocate(ldlib, slen>512 ? slen : 512);

    if(ldlib->bufpos == 0) {
        ldlib->bufpos = snprintf(ldlib->buf, ldlib->bufsiz, "LD_LIBRARY_PATH=%s", s);
    } else {
        size_t remainbuf = ldlib->bufsiz - ldlib->bufpos;
        while(strlen(s) > remainbuf) {
            ldlib->buf = realloc(ldlib->buf, ldlib->bufsiz*2);
            ldlib->bufsiz *= 2;
            remainbuf = ldlib->bufsiz - ldlib->bufpos;
        }
        ldlib->bufpos += snprintf(ldlib->buf + ldlib->bufpos, remainbuf, ":%s", s);
    }
}

PRIVATE void HxDestroyLdlib(HxLdlib *ldlib) {
    if(ldlib->buf) {
        free(ldlib->buf);
        ldlib->buf = NULL;
        ldlib->bufpos = 0;
        ldlib->bufsiz = 0;
    }
}
