#include <dirent.h>
#include <dlfcn.h>

#include "hxroot.h"

static int (*scandir_real)(const char *restrict dirp, struct dirent ***restrict namelist, typeof(int (const struct dirent *)) *filter, typeof(int (const struct dirent **, const struct dirent **)) *compar);
int scandir(const char *restrict dirp, struct dirent ***restrict namelist, typeof(int (const struct dirent *)) *filter, typeof(int (const struct dirent **, const struct dirent **)) *compar) {
    if(!scandir_real) scandir_real = dlsym(RTLD_NEXT, "scandir");
    HxInit();

    int len = HxL(dirp);
    char pathbuf[len];
    const char *new_dirp = HxExpandPath(pathbuf, dirp);

    return scandir_real(new_dirp, namelist, filter, compar);
}

static int (*scandirat_real)(int dirfd, const char *restrict dirp, struct dirent ***restrict namelist, typeof(int (const struct dirent *)) *filter, typeof(int (const struct dirent **, const struct dirent **)) *compar);
int scandirat(int dirfd, const char *restrict dirp, struct dirent ***restrict namelist, typeof(int (const struct dirent *)) *filter, typeof(int (const struct dirent **, const struct dirent **)) *compar) {
    if(!scandirat_real) scandirat_real = dlsym(RTLD_NEXT, "scandirat");
    HxInit();

    int len = HxL(dirp);
    char pathbuf[len];
    const char *new_dirp = HxExpandPath(pathbuf, dirp);

    return scandirat_real(dirfd, new_dirp, namelist, filter, compar);
}
