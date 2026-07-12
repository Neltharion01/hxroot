#include <stdlib.h>
#include <pwd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <dlfcn.h>

#include "hxroot.h"

static FILE *HxPwdFile;
static struct passwd HxPwd;
static char HxPwdBuf[256];

int getpwent_r(struct passwd *restrict pwbuf, char *restrict buf, size_t size, struct passwd **restrict pwbufp) {
    if(!HxPwdFile) HxPwdFile = fopen("/etc/passwd", "r");
    // still null
    if(!HxPwdFile) return errno;

    return fgetpwent_r(HxPwdFile, pwbuf, buf, size, pwbufp);
}

struct passwd *getpwent(void) {
    struct passwd *pwbufp = NULL;
    int ret = getpwent_r(&HxPwd, HxPwdBuf, sizeof(HxPwdBuf), &pwbufp);
    if(ret != 0) {
        errno = ret;
        return NULL;
    }

    return pwbufp;
}

void setpwent(void) {
    if(HxPwdFile) rewind(HxPwdFile);
}

void endpwent(void) {
    if(HxPwdFile) {
        fclose(HxPwdFile);
        HxPwdFile = NULL;
    }
}

int (*getpwnam_r_real)(const char *name, struct passwd *pwd, char *buf, size_t size, struct passwd **restrict result);
int getpwnam_r(const char *name, struct passwd *pwd, char *buf, size_t size, struct passwd **restrict result) {
    // Lazy init
    if(!getpwnam_r_real) getpwnam_r_real = dlsym(RTLD_NEXT, "getpwnam_r");
    HxInit();

    // Debug
    if(HxDebug) eprintf("getpwnam_r(\"%s\")\n", name);

    // Try to open guest passwd
    FILE *f = fopen("/etc/passwd", "r");
    if(!f) {
        *result = NULL;
        return errno;
    }

    // Iterate until entry found
    int ret = fgetpwent_r(f, pwd, buf, size, result);
    while(ret == 0 && strcmp(pwd->pw_name, name) != 0) {
        ret = fgetpwent_r(f, pwd, buf, size, result);
    }

    // Not found? Try to forward request to host
    if(ret != 0) ret = getpwnam_r_real(name, pwd, buf, size, result);

    // Still not found -> return null
    if(ret != 0) *result = 0;
    return ret;
}

// This is the same as previous, but for uid
int (*getpwuid_r_real)(uid_t uid, struct passwd *restrict pwd, char *buf, size_t size, struct passwd **restrict result);
int getpwuid_r(uid_t uid, struct passwd *restrict pwd, char *buf, size_t size, struct passwd **restrict result) {
    // Lazy init
    if(!getpwuid_r_real) getpwuid_r_real = dlsym(RTLD_NEXT, "getpwuid_r");
    HxInit();

    // Debug
    if(HxDebug) eprintf("getpwuid_r(%d)\n", uid);

    // Try to open guest passwd
    FILE *f = fopen("/etc/passwd", "r");
    if(!f) {
        *result = NULL;
        return errno;
    }

    // Iterate until entry found
    int ret = fgetpwent_r(f, pwd, buf, size, result);
    while(ret == 0 && pwd->pw_uid != uid) {
        ret = fgetpwent_r(f, pwd, buf, size, result);
    }

    // Not found? Try to forward request to host
    if(ret != 0) ret = getpwuid_r_real(uid, pwd, buf, size, result);

    // Still not found -> return null
    if(ret != 0) *result = 0;
    return ret;
}

struct passwd *getpwnam(const char *name) {
    struct passwd *result = NULL;
    int ret = getpwnam_r(name, &HxPwd, HxPwdBuf, sizeof(HxPwdBuf), &result);
    if(ret != 0) {
        errno = ret;
        return NULL;
    }
    return result;
}

struct passwd *getpwuid(uid_t uid) {
    struct passwd *result = NULL;
    int ret = getpwuid_r(uid, &HxPwd, HxPwdBuf, sizeof(HxPwdBuf), &result);
    if(ret != 0) {
        errno = ret;
        return NULL;
    }
    return result;
}

int (*getpw_real)(uid_t uid, char *buf);
int getpw(uid_t uid, char *buf) {
    if(!getpw_real) getpw_real = dlsym(RTLD_NEXT, "getpw");
    HxInit();

    if(HxDebug) eprintf("getpw(%d)\n", uid);

    struct passwd pwd = {0};
    struct passwd *result = NULL;
    char pwbuf[256];

    int ret = getpwuid_r(uid, &pwd, pwbuf, 256, &result);
    if(ret != 0) {
        return getpw_real(uid, buf);
    } else {
        snprintf(buf, 256, "%s:%s:%d:%d:%s:%s:%s", pwd.pw_name, pwd.pw_passwd, pwd.pw_uid, pwd.pw_gid, pwd.pw_gecos, pwd.pw_dir, pwd.pw_shell);
        return 0;
    }
}
