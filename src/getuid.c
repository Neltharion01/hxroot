#include <sys/types.h>
#include <dlfcn.h>

#include "hxroot.h"

static uid_t (*getuid_real)(void);
uid_t getuid(void) {
    HxInit();
    if(HxUid != -1) return HxUid;

    if(!getuid_real) getuid_real = dlsym(RTLD_NEXT, "getuid");
    return getuid_real();
}

static uid_t (*geteuid_real)(void);
uid_t geteuid(void) {
    HxInit();
    if(HxUid != -1) return HxUid;

    if(!geteuid_real) geteuid_real = dlsym(RTLD_NEXT, "geteuid");
    return geteuid_real();
}

static gid_t (*getgid_real)(void);
gid_t getgid(void) {
    HxInit();
    if(HxGid != -1) return HxGid;

    if(!getgid_real) getgid_real = dlsym(RTLD_NEXT, "getgid");
    return getgid_real();
}

static gid_t (*getegid_real)(void);
gid_t getegid(void) {
    HxInit();
    if(HxGid != -1) return HxGid;

    if(!getegid_real) getegid_real = dlsym(RTLD_NEXT, "getegid");
    return getegid_real();
}

int (*getresuid_real)(uid_t *ruid, uid_t *euid, uid_t *suid);
int getresuid(uid_t *ruid, uid_t *euid, uid_t *suid) {
    HxInit();
    if(HxUid != -1) { *ruid = HxUid; *euid = HxUid; *suid = HxUid; return 0; }

    if(!getresuid_real) getresuid_real = dlsym(RTLD_NEXT, "getresuid");
    return getresuid_real(ruid, euid, suid);
}

int (*getresgid_real)(gid_t *rgid, gid_t *egid, gid_t *sgid);
int getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid) {
    HxInit();
    if(HxGid != -1) { *rgid = HxGid; *egid = HxGid; *sgid = HxGid; return 0; }

    if(!getresgid_real) getresgid_real = dlsym(RTLD_NEXT, "getresgid");
    return getresgid_real(rgid, egid, sgid);
}
