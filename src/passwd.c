#include <stdlib.h>
#include <pwd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "hxroot.h"

static FILE *HxPwdFile;

int getpwent_r(struct passwd *restrict pwbuf, char *restrict buf, size_t size, struct passwd **restrict pwbufp) {
    if(!HxPwdFile) HxPwdFile = fopen("/etc/passwd", "r");
    // still null
    if(!HxPwdFile) return errno;

    return fgetpwent_r(HxPwdFile, pwbuf, buf, size, pwbufp);
}

struct passwd *getpwent(void) {
    static struct passwd HxPwd;
    static char HxPwdBuf[256];

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

int getpwnam_r(const char *name, struct passwd *pwd, char *buf, size_t size, struct passwd **restrict result) {
    FILE *f = fopen("/etc/passwd", "r");
    if(!f) {
        *result = NULL;
        return errno;
    }

    int ret = fgetpwent_r(f, pwd, buf, size, result);
    while(ret == 0 && strcmp(pwd->pw_name, name) != 0) {
        ret = fgetpwent_r(f, pwd, buf, size, result);
    }

    if(ret != 0) *result = 0;

    fclose(f);
    return ret;
}

int getpwuid_r(uid_t uid, struct passwd *restrict pwd, char *buf, size_t size, struct passwd **restrict result) {
    FILE *f = fopen("/etc/passwd", "r");
    if(!f) {
        *result = NULL;
        return errno;
    }

    int ret = fgetpwent_r(f, pwd, buf, size, result);
    while(ret == 0 && pwd->pw_uid != uid) {
        ret = fgetpwent_r(f, pwd, buf, size, result);
    }

    if(ret != 0) *result = 0;

    fclose(f);
    return ret;
}

struct passwd *getpwnam(const char *name) {
    static struct passwd HxPwd;
    static char HxPwdBuf[256];

    struct passwd *result = NULL;
    int ret = getpwnam_r(name, &HxPwd, HxPwdBuf, sizeof(HxPwdBuf), &result);
    if(ret != 0) {
        errno = ret;
        return NULL;
    }
    return result;
}

struct passwd *getpwuid(uid_t uid) {
    static struct passwd HxPwd;
    static char HxPwdBuf[256];

    struct passwd *result = NULL;
    int ret = getpwuid_r(uid, &HxPwd, HxPwdBuf, sizeof(HxPwdBuf), &result);
    if(ret != 0) {
        errno = ret;
        return NULL;
    }
    return result;
}

int getpw(uid_t uid, char *buf) {
    struct passwd pwd = {0};
    struct passwd *result = NULL;
    char pwbuf[256];

    int ret = getpwuid_r(uid, &pwd, pwbuf, 256, &result);
    if(ret != 0) {
        errno = ret;
        return -1;
    } else {
        snprintf(buf, 256, "%s:%s:%d:%d:%s:%s:%s", pwd.pw_name, pwd.pw_passwd, pwd.pw_uid, pwd.pw_gid, pwd.pw_gecos, pwd.pw_dir, pwd.pw_shell);
        return 0;
    }
}
