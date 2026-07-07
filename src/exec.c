#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <linux/limits.h>
#include <stdarg.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <elf.h>
#include <errno.h>

#include "hxroot.h"
#include "elfutil.h"
#include "ldlib.h"

extern char **environ;

static bool HxIsShebang(char *buf, int nr) {
    return nr >= 2 && buf[0] == '#' && buf[1] == '!';
}

static bool HxIsElf(const char *buf, int nr) {
    return nr >= 4 &&
        buf[0] == 0x7f && buf[1] == 'E' &&
        buf[2] == 'L' && buf[3] == 'F';
}

static bool HxIsElf32(char *buf, int nr) {
    return nr >= (int)sizeof(Elf32_Ehdr) && buf[EI_CLASS] == ELFCLASS32;
}

static bool HxIsElf64(char *buf, int nr) {
    return nr >= (int)sizeof(Elf64_Ehdr) && buf[EI_CLASS] == ELFCLASS64;
}

static size_t HxCountArgv(char *const argv[]) {
    size_t count = 0;
    while(argv[count] != 0) count += 1;
    return count;
}

static int HxHandleShebang(char *shebang, const char *new_path, char *const argv[], char *const envp[]) {
    int argc = HxCountArgv(argv);

    char *interp = shebang + 2;

    // Skip spaces in the beginning
    while(*interp == ' ') interp++;
    if(*interp == '\0') {
        interp = "/bin/sh";
    }

    char *arg = NULL;
    char *space = strchr(interp, ' ');

    if(space) {
        *space = '\0';
        space += 1;

        // Skip spaces until we find arg
        while(*space == ' ') space++;

        if(*space != '\0') {
            arg = space;
        }
    }

    char *interp_name = strrchr(interp, '/');
    if(!interp_name) interp_name = interp;
    else interp_name += 1;

    // Left branch needs interp_name, path, ..argv[1:] and '\0' (argc+2)
    // Right branch needs interp_name, arg, path, argv[1:] and '\0' (argc+3)
    char *new_argv[argc+3];
    if(!arg) {
        // Don't have arg
        new_argv[0] = interp_name;
        new_argv[1] = (char*)new_path;
        // Original argv[0] is script's name, we skip it
        memcpy(new_argv+2, argv+1, sizeof(char*) * argc);
    } else {
        // Has arg
        new_argv[0] = interp_name;
        new_argv[1] = arg;
        new_argv[2] = (char*)new_path;
        // Skip argv[0] too
        memcpy(new_argv+3, argv+1, sizeof(char*) * argc);
    }

    return execve(interp, new_argv, envp);
}

static int (*execve_real)(const char *path, char *const argv[], char *const envp[]);
static int HxHandleElf64(const char *new_path, char *const argv[], char *const envp[], struct HxElfInfo *info) {
    int argc = HxCountArgv(argv);
    const char *interp = NULL;

    // Declare linker buf in outside block
    int len = HxL(info->interp);
    char pathbuf[len];

    if(HxLinker) {
        interp = HxLinker;
    } else {
        // HxLinker not defined = expand interp path
        interp = HxExpandPath(pathbuf, info->interp);
    }

    // Declare envp copy in outside block
    int envc = HxCountArgv(envp);
    char *new_envp[envc+2];
    memcpy(new_envp, envp, (envc+1)*sizeof(char*));
    AUTO_FREE_LDLIB HxLdlib new_ldlib = {0};

    if(info->rpath || info->runpath) {
        // Find LD_LIBRARY_PATH
        int ldlib_idx = -1;
        for(int i = 0; i <= envc; i++) {
            if(strncmp(envp[i], "LD_LIBRARY_PATH=", strlen("LD_LIBRARY_PATH=")) == 0) {
                ldlib_idx = i;
                break;
            }
        }

        if(ldlib_idx != -1) {
            HxLdlib_set(&new_ldlib, envp[ldlib_idx]);
        }

        if(info->rpath) {
            char *saveptr = NULL;
            char *tok = strtok_r(info->rpath, ":", &saveptr);
            while(tok) {
                len = HxL(tok);
                if(len) {
                    char tokbuf[len];
                    HxExpandPath(tokbuf, tok);
                    // TODO append only if not already contains
                    HxLdlib_append(&new_ldlib, tokbuf);
                }
                tok = strtok_r(NULL, ":", &saveptr);
            }
        }
        if(info->runpath) {
            char *saveptr = NULL;
            char *tok = strtok_r(info->runpath, ":", &saveptr);
            while(tok) {
                len = HxL(tok);
                if(len) {
                    char tokbuf[len];
                    HxExpandPath(tokbuf, tok);
                    HxLdlib_append(&new_ldlib, tokbuf);
                }
                tok = strtok_r(NULL, ":", &saveptr);
            }
        }

        if(new_ldlib.buf) {
            if(ldlib_idx != -1) {
                new_envp[ldlib_idx] = new_ldlib.buf;
            } else {
                new_envp[envc] = new_ldlib.buf;
                new_envp[envc+1] = '\0';
            }
        }
    }

    // argc == 0: ld-linux-aarch64.so.1 <new_path> \0
    // argc >= 1: ld-linux-aarch64.so.1 --argv0 <argv[0]> <new_path> <argv[1:]> \0
    char *new_argv[argc+4];
    if(argv[0] == 0) {
        // Empty argv
        new_argv[0] = (char*)interp;
        new_argv[1] = (char*)new_path;
        new_argv[2] = 0;
        new_path = interp;
    } else {
        // Copy argv
        new_argv[0] = (char*)interp;
        new_argv[1] = "--argv0";
        new_argv[2] = argv[0];
        new_argv[3] = (char*)new_path;
        int i = 4;
        for(int y = 1; argv[y] != 0; y++) {
            new_argv[i] = argv[y];
            i += 1;
        }
        new_argv[i] = 0;
        new_path = interp;
    }

    if(HxDebug) {
        eprintf("execve(\"%s\", [", new_path);
        eprintf("\"%s\"", new_argv[0]);
        for(char *const *cur = new_argv+1; *cur != 0; cur++) {
            eprintf(", \"%s\"", *cur);
        }
        eprintf("], %p)\n", envp);
    }

    return execve_real(new_path, new_argv, new_envp);
}

static int HxHandleProot(const char *path, char *const argv[], char *const envp[]) {
    int argc = HxCountArgv(argv);
    int envc = HxCountArgv(envp);

    if(!HxProot) {
        eprintf("HxProot variable is unset\n");
        errno = ENOENT;
        return -1;
    }

    char *new_envp[envc+1];
    memset(new_envp, 0, sizeof(char*) * (envc+1));
    int new_envp_idx = 0;
    for(int i = 0; envp[i] != 0; i++) {
        if(strncmp("LD_PRELOAD=", envp[i], 11) != 0) {
            new_envp[new_envp_idx] = envp[i];
            new_envp_idx += 1;
        }
    }

    // LD_PRELOAD= proot -r $HxRoot -w $PWD -b $HxBinds -i $HxUid:$HxGid <new_path> <argv[1:]> \0
    char *new_argv[argc+8+HxBindsLen*2];
    new_argv[0] = "proot";
    new_argv[1] = "-r";
    new_argv[2] = HxRoot;

    char cwd[PATH_MAX];
    if(getcwd(cwd, PATH_MAX) == NULL) return -1;
    new_argv[3] = "-w";
    new_argv[4] = cwd;

    int i = 5;
    for(int bind = 0; bind < HxBindsLen; bind++) {
        // Add -b opts
        new_argv[i] = "-b";
        new_argv[i+1] = HxBinds[bind];
        i += 2;
    }

    new_argv[i] = "-i";
    char ids[32];
    snprintf(ids, 32, "%d:%d", getuid(), getgid());
    new_argv[i+1] = ids;
    i += 2;

    new_argv[i] = (char*)path;
    i += 1;

    // Copy argv
    for(size_t y = 1; y <= (size_t)argc; y++) {
        new_argv[i] = argv[y];
        i += 1;
    }

    return execve_real(HxProot, new_argv, new_envp);
}

int execve(const char *path, char *const argv[], char *const envp[]) {
    if(!execve_real) execve_real = dlsym(RTLD_NEXT, "execve");
    HxInit();

    int len = HxL(path);
    char pathbuf[len];
    const char *new_path = HxExpandPath(pathbuf, path);

    AUTO_CLOSE int fd = open(path, O_RDONLY|O_CLOEXEC);
    if(fd == -1) return -1;

    char buf[4096];
    ssize_t nr = read(fd, buf, 4096-1);
    if(nr == -1) return -1;

    if(HxIsShebang(buf, nr)) {
        char *newline = memchr(buf, '\n', nr);
        if(!newline) newline = buf + nr;
        *newline = '\0';
        return HxHandleShebang(buf, path, argv, envp);
    } else if(HxIsElf(buf, nr) && HxIsElf64(buf, nr)) {
        Elf64_Ehdr ehdr;
        memcpy(&ehdr, buf, sizeof(Elf64_Ehdr));

        AUTO_FREE_ELFINFO struct HxElfInfo out = {0};
        if(HxParseElfInfo64(fd, &ehdr, &out) == -1) return -1;

        if(out.interp) {
            return HxHandleElf64(new_path, argv, envp, &out);
        } else {
            return HxHandleProot(path, argv, envp);
        }
    } else if(HxIsElf(buf, nr) && HxIsElf32(buf, nr)) {
        // TODO needs special handling
        return HxHandleProot(path, argv, envp);
    } else {
        errno = ENOEXEC;
        return -1;
    }
}

int execv(const char *path, char *const argv[]) {
    return execve(path, argv, environ);
}

static int HxShellExecute(const char *file, char *const argv[], char *const envp[]) {
    int argc = HxCountArgv(argv);

    // Empty argv: /bin/sh <file> \0
    // argv>0: /bin/sh <argv>
    int argvlen;
    if(argc == 0) {
        argvlen = 3;
    } else {
        argvlen = argc + 2;
    }

    char *new_argv[argvlen];
    if(argc == 0) {
        new_argv[0] = "/bin/sh";
        new_argv[1] = (char*)file;
        new_argv[2] = 0;
    } else {
        new_argv[0] = "/bin/sh";
        memcpy(new_argv + 1, argv, sizeof(char*)*(argc+1));
    }

    return execve("/bin/sh", new_argv, envp);
}

// From musl
int execvpe(const char *file, char *const argv[], char *const envp[]) {
    const char *p, *z, *path = getenv("PATH");
    size_t l, k;
    int seen_eacces = 0;

    errno = ENOENT;
    if (!*file) return -1;

    if (strchr(file, '/')) {
        execve(file, argv, envp);
        if(errno == ENOEXEC) return HxShellExecute(file, argv, envp);
    }

    if (!path) path = "/usr/local/bin:/bin:/usr/bin";
    k = strnlen(file, NAME_MAX+1);
    if (k > NAME_MAX) {
        errno = ENAMETOOLONG;
        return -1;
    }
    l = strnlen(path, PATH_MAX-1)+1;

    for(p=path; ; p=z) {
        char b[l+k+1];
        z = strchrnul(p, ':');
        if (z-p >= l) {
            if (!*z++) break;
            continue;
        }
        memcpy(b, p, z-p);
        b[z-p] = '/';
        memcpy(b+(z-p)+(z>p), file, k+1);
        execve(b, argv, envp);
        if(errno == ENOEXEC) return HxShellExecute(b, argv, envp);
        switch (errno) {
        case EACCES:
            seen_eacces = 1;
        case ENOENT:
        case ENOTDIR:
            break;
        default:
            return -1;
        }
        if (!*z++) break;
    }
    if (seen_eacces) errno = EACCES;
    return -1;
}

int execvp(const char *file, char *const argv[]) {
    return execvpe(file, argv, environ);
}

int execl(const char *path, const char *arg, ...) {
    size_t argc = 1;

    va_list args;
    va_start(args, arg);
    while(va_arg(args, const char*) != 0) argc += 1;
    va_end(args);

    char *argv[argc+1];

    va_start(args, arg);
    argv[0] = (char*)arg;
    for(size_t i = 1; i <= argc; i++) argv[i] = va_arg(args, char*);
    va_end(args);

    return execve(path, argv, environ);
}

int execle(const char *path, const char *arg, ...) {
    size_t argc = 1;

    va_list args;
    va_start(args, arg);
    while(va_arg(args, const char*) != 0) argc += 1;
    va_end(args);

    char *argv[argc+1];
    char **envp;

    va_start(args, arg);
    argv[0] = (char*)arg;
    for(size_t i = 1; i <= argc; i++) argv[i] = va_arg(args, char*);
    envp = va_arg(args, char**);
    va_end(args);

    return execve(path, argv, envp);
}

int execlp(const char *file, const char *arg, ...) {
    size_t argc = 1;

    va_list args;
    va_start(args, arg);
    while(va_arg(args, const char*) != 0) argc += 1;
    va_end(args);

    char *argv[argc+1];

    va_start(args, arg);
    argv[0] = (char*)arg;
    for(size_t i = 1; i <= argc; i++) argv[i] = va_arg(args, char*);
    va_end(args);

    return execvpe(file, argv, environ);
}

int execveat(int dirfd, const char *path, char *const argv[], char *const envp[], int flags) {
    eprintf("UNIMPLEMENTED SHIT! execveat\n"); abort();
}
