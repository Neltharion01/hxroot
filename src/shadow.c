#include <shadow.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "util.h"

static FILE *HxSpFile;

int getspent_r(struct spwd *spbuf,
               char *buf, size_t size,
               struct spwd **spbufp)
{
    if(!HxSpFile) HxSpFile = fopen("/etc/shadow", "r");
    // still null
    if(!HxSpFile) return errno;

    return fgetspent_r(HxSpFile, spbuf, buf, size, spbufp);
}

struct spwd *getspent(void) {
    if(!HxSpFile) HxSpFile = fopen("/etc/shadow", "r");
    // still null
    if(!HxSpFile) return NULL;

    return fgetspent(HxSpFile);
}

void setspent(void) {
    if(HxSpFile) rewind(HxSpFile);
}

void endspent(void) {
    if(HxSpFile) {
        fclose(HxSpFile);
        HxSpFile = NULL;
    }
}

int lckpwdf(void) {
    return 0;
}
int ulckpwdf(void) {
    return 0;
}

int getspnam_r(const char *name, struct spwd *spbuf,
               char *buf, size_t size,
               struct spwd **spbufp)
{
    // Try to open guest shadow
    FILE *f = fopen("/etc/shadow", "r");
    if(!f) {
        *spbufp = NULL;
        return errno;
    }

    // Iterate until entry found
    int ret = fgetspent_r(f, spbuf, buf, size, spbufp);
    while(ret == 0 && strcmp(spbuf->sp_namp, name) != 0) {
        ret = fgetspent_r(f, spbuf, buf, size, spbufp);
    }

    // Not found -> return null
    if(ret != 0) *spbufp = 0;

    fclose(f);
    return ret;
}

struct spwd *getspnam(const char *name) {
    static struct spwd HxSp;
    static char HxSpBuf[256];

    struct spwd *result = NULL;
    int ret = getspnam_r(name, &HxSp, HxSpBuf, sizeof(HxSpBuf), &result);
    if(ret != 0) {
        errno = ret;
        return NULL;
    }
    return result;
}
