#include <stdlib.h>
#include <stdio.h>

#include "hxroot.h"

static void HxSetUid(uid_t u) {
    if(HxUid == (int)u) return;
    HxUid = u;
    char s[16];
    snprintf(s, 16, "%d", u);
    setenv("HxUid", s, 1);
}

static void HxSetGid(gid_t g) {
    if(HxGid == (int)g) return;
    HxGid = g;
    char s[16];
    snprintf(s, 16, "%d", g);
    setenv("HxGid", s, 1);
}

int setuid(uid_t uid) { HxInit(); HxSetUid(uid); return 0; }
int setgid(gid_t gid) { HxInit(); HxSetGid(gid); return 0; }
int seteuid(uid_t euid) { HxInit(); HxSetUid(euid); return 0; }
int setegid(gid_t egid) { HxInit(); HxSetGid(egid); return 0; }

int setreuid(uid_t ruid, uid_t euid) {
    HxInit();
    if(ruid != -1) HxSetUid(ruid);
    return 0;
}

int setregid(gid_t rgid, gid_t egid) {
    HxInit();
    if(rgid != -1) HxSetGid(rgid);
    return 0;
}

int setresuid(uid_t ruid, uid_t euid, uid_t suid) {
    HxInit();
    if(ruid != -1) HxSetUid(ruid);
    return 0;
}

int setresgid(gid_t rgid, gid_t egid, gid_t sgid) {
    HxInit();
    if(rgid != -1) HxSetGid(rgid);
    return 0;
}
