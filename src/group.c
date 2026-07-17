#include <grp.h>
#include <stdio.h>
#include <dlfcn.h>
#include <errno.h>
#include <string.h>

#include "hxroot.h"

static FILE *HxGrpFile;

int getgrent_r(struct group *restrict gbuf,
               char *buf, size_t size,
               struct group **restrict gbufp)
{
    if(!HxGrpFile) HxGrpFile = fopen("/etc/group", "r");
    // still null
    if(!HxGrpFile) return errno;

    return fgetgrent_r(HxGrpFile, gbuf, buf, size, gbufp);
}

struct group *getgrent(void) {
    static struct group HxGrp;
    static char HxGrpBuf[256];

    struct group *gbufp = NULL;
    int ret = getgrent_r(&HxGrp, HxGrpBuf, sizeof(HxGrpBuf), &gbufp);
    if(ret != 0) {
        errno = ret;
        return NULL;
    }

    return gbufp;
}

void setgrent(void) {
    if(HxGrpFile) rewind(HxGrpFile);
}

void endgrent(void) {
    if(HxGrpFile) {
        fclose(HxGrpFile);
        HxGrpFile = NULL;
    }
}

int getgrnam_r(const char *restrict name, struct group *restrict grp,
               char *buf, size_t size,
               struct group **restrict result)
{
    FILE *f = fopen("/etc/group", "r");
    if(!f) {
        *result = NULL;
        return errno;
    }

    int ret = fgetgrent_r(f, grp, buf, size, result);
    while(ret == 0 && strcmp(grp->gr_name, name) != 0) {
        ret = fgetgrent_r(f, grp, buf, size, result);
    }

    if(ret != 0) *result = 0;

    fclose(f);
    return ret;
}

int getgrgid_r(gid_t gid, struct group *restrict grp,
               char *buf, size_t size,
               struct group **restrict result)
{
    FILE *f = fopen("/etc/group", "r");
    if(!f) {
        *result = NULL;
        return errno;
    }

    int ret = fgetgrent_r(f, grp, buf, size, result);
    while(ret == 0 && grp->gr_gid != gid) {
        ret = fgetgrent_r(f, grp, buf, size, result);
    }

    if(ret != 0) *result = 0;

    fclose(f);
    return ret;
}

struct group *getgrnam(const char *name) {
    static struct group grp;
    static char buf[256];

    struct group *result = NULL;
    int ret = getgrnam_r(name, &grp, buf, sizeof(buf), &result);
    if(ret != 0) {
        errno = ret;
        return NULL;
    }

    return result;
}

struct group *getgrgid(gid_t gid) {
    static struct group grp;
    static char buf[256];

    struct group *result = NULL;
    int ret = getgrgid_r(gid, &grp, buf, sizeof(buf), &result);
    if(ret != 0) {
        errno = ret;
        return NULL;
    }

    return result;
}

int getgrouplist(const char *user, gid_t group, gid_t *groups, int *ngroups) {
    FILE *f = fopen("/etc/group", "r");
    if(!f) {
        *ngroups = 0;
        return 0;
    }

    struct group grp;
    struct group *result = NULL;
    char buf[256];

    int i = 0;

    // Include first group
    if(i < *ngroups) groups[i] = group;
    i += 1;

    int ret = fgetgrent_r(f, &grp, buf, sizeof(buf), &result);
    while(ret == 0 && result) {
        for(char **mem = grp.gr_mem; *mem; mem++) {
            if(strcmp(*mem, user) == 0 && grp.gr_gid != group) {
                if(i < *ngroups) groups[i] = grp.gr_gid;
                i += 1;
            }
        }

        ret = fgetgrent_r(f, &grp, buf, sizeof(buf), &result);
    }

    ret = i;
    if(i > *ngroups) ret = -1;

    *ngroups = i;
    fclose(f);
    return ret;
}
