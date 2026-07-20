#define _GNU_SOURCE
#include <unistd.h>
#include <string.h>
#include <stddef.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <linux/prctl.h>
#include <stdlib.h>

#include "hxroot.h"

// Only glibc passes argv to initializers
// Can't be done on other libcs
#ifdef __GLIBC__
__attribute__((constructor))
static int HxFixupArgv(int argc, char *argv[], char *envp[]) {
    // Empty argv?
    if(!argv[0]) return 0;

    // Read real cmdline
    char buf[4096];
    AUTO_CLOSE int fd = open("/proc/self/cmdline", O_RDONLY);
    if(fd == -1) return 1;

    ssize_t ret = read(fd, buf, sizeof(buf));
    if(ret == -1) return 1;

    // Not the best thing to do, but probably works
    // Find real cmdline address in memory by searching for it backwards
    // Also, can search the stack upwards, but backwards is faster
    char *real_argv = argv[0];
    while(strcmp(real_argv, buf) != 0) real_argv -= 1;

    // Optimistic check
    if(real_argv == argv[0]) return 0;

    // Save real exe path
    // Unfortunately we have to parse argv manually for that...
    char *real_exe = real_argv;
    if(strstr(real_exe, "ld-linux-")) {
        // Skip 1st arg
        real_exe = strchr(real_exe, 0) + 1;
        if(strcmp(real_exe, "--argv0") == 0) {
            // Skip another arg
            real_exe = strchr(real_exe, 0) + 1;
            real_exe = strchr(real_exe, 0) + 1;
        }
        real_exe = strdupa(real_exe);
        HxUnexpandPath(real_exe);
        HxExe = realpath(real_exe, NULL);
        if(HxExe) {
            HxUnexpandPath(HxExe);
            HxExeLen = strlen(HxExe);
        }
    }

    // Move argv[0]
    size_t argv0len = strlen(argv[0]) + 1;
    memmove(real_argv, argv[0], argv0len);
    argv[0] = real_argv;

    size_t argvlen = 0;

    // Fix other args, if any
    if(argv[1]) {
        // Find last arg
        char **lastarg;
        for(lastarg = argv; *lastarg; lastarg++) {}
        lastarg -= 1;

        // Move argv[1] and others backwards, to real_argv+argv0len (where argv[1] should start)
        argvlen = *lastarg - argv[1] + strlen(*lastarg) + 1;
        memmove(real_argv+argv0len, argv[1], argvlen);

        // Calculate difference between previous and new location
        ptrdiff_t diff = argv[1] - real_argv - argv0len;
        // Fix up argv elements to point to new location
        for(char **cur = argv+1; *cur; cur++) *cur -= diff;
    }

    // Also fix these globals
    program_invocation_name = argv[0];
    char *shortname = strrchr(argv[0], '/');
    if(!shortname) shortname = argv[0];
    program_invocation_short_name = shortname;

    // Update thread name
    if(prctl(PR_SET_NAME, shortname) == -1) {
        // Ignore error
    }

    // Zero the remaining part
    memset(argv[0] + argv0len + argvlen, 0, ret - argv0len - argvlen);

    return 0;
}
#endif
