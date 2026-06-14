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

extern char **environ;

static _Thread_local char HxEnvPathBuf[PATH_MAX];
static const char *HxEnvPathFind(const char *binary_name) {
    // If the binary name contains a slash, do not search PATH
    if (strchr(binary_name, '/') != NULL) {
        if (access(binary_name, X_OK) == 0) {
            return binary_name;
        }
        return NULL;
    }

    // Retrieve the PATH environment variable
    char *path_env = getenv("PATH");
    if (!path_env) return NULL;

    // Duplicate PATH because strtok modifies the string
    char *path_copy = strdup(path_env);
    if (!path_copy) return NULL;

    char *token = strtok(path_copy, ":");

    while (token != NULL) {
        // Construct the potential absolute path
        snprintf(HxEnvPathBuf, PATH_MAX-1, "%s/%s", token, binary_name);

        // Check if the file exists and is executable
        if (access(HxEnvPathBuf, X_OK) == 0) {
            free(path_copy);
            return HxEnvPathBuf;
        }

        // Move to the next directory in PATH
        token = strtok(NULL, ":");
    }

    free(path_copy);
    return NULL;
}

static bool HxIsShebang(char *buf, int nr) {
    return nr >= 2 && buf[0] == '#' && buf[1] == '!';
}

static bool HxIsElf(const char *buf, int nr) {
    return nr >= EI_NIDENT &&
        buf[0] == 0x7f && buf[1] == 'E' &&
        buf[2] == 'L' && buf[3] == 'F';
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

    char *space = strchr(shebang, ' ');
    if(space) *space = '\0';

    char *interp = shebang + 2;
    char *interp_name = strrchr(interp, '/');
    if(!interp_name) interp_name = interp;
    else interp_name += 1;

    // Left branch needs interp_name, path, ..argv[1:] and '\0' (argc+2)
    // Right branch needs interp_name, arg, path, argv[1:] and '\0' (argc+3)
    char *new_argv[argc+3];
    if(!space) {
        // Don't have arg
        new_argv[0] = interp_name;
        new_argv[1] = (char*)new_path;
        // Original argv[0] is script's name, we skip it
        memcpy(new_argv+2, argv+1, sizeof(char*) * argc);
    } else {
        char *arg = space + 1;
        // Has arg
        new_argv[0] = interp_name;
        new_argv[1] = arg;
        new_argv[2] = (char*)new_path;
        // Skip argv[0] too
        memcpy(new_argv+3, argv+1, sizeof(char*) * argc);
    }

    return execve(interp, new_argv, envp);
}

static int HxHasInterp(int fd, Elf64_Ehdr *ehdr) {
    if(ehdr->e_shnum * sizeof(Elf64_Shdr) > 4096) {
        // We aren't going to allocate that much
        errno = E2BIG;
        return -1;
    }

    Elf64_Shdr sections[ehdr->e_shnum];

    off_t off = lseek(fd, ehdr->e_shoff, SEEK_SET);
    if(off == -1) return -1;
    ssize_t nr = read(fd, sections, sizeof(Elf64_Shdr) * ehdr->e_shnum);
    if(nr == -1) return -1;
    if(nr != (ssize_t)sizeof(Elf64_Shdr) * ehdr->e_shnum) { errno = ENODATA; return -1; }

    Elf64_Shdr *strtab = &sections[ehdr->e_shstrndx];
    if(strtab->sh_size > 4096) {
        errno = E2BIG;
        return -1;
    }
    char strtab_s[strtab->sh_size];

    off = lseek(fd, strtab->sh_offset, SEEK_SET);
    if(off == -1) return -1;
    nr = read(fd, strtab_s, strtab->sh_size);
    if(nr == -1) return -1;
    if(nr != (ssize_t)strtab->sh_size) { errno = ENODATA; return -1; }

    for(int i = 0; i < ehdr->e_shnum; i++) {
        char *name = strtab_s + sections[i].sh_name;
        if(strcmp(name, ".interp") == 0) {
            return true;
        }
    }

    return false;
}

static int (*execve_real)(const char *path, char *const argv[], char *const envp[]);
static int HxHandleElf(Elf64_Ehdr *ehdr, const char *new_path, char *const argv[], char *const envp[]) {
    int argc = HxCountArgv(argv);

    // argc == 0: ld-linux-aarch64.so.1 <new_path> \0
    // argc >= 1: ld-linux-aarch64.so.1 --argv0 <argv[0]> <new_path> <argv[1:]> \0
    char *new_argv[argc+4];
    if(!HxLinker) {
        // HxLinker not defined = execute as is
        memcpy(new_argv, argv, sizeof(char*) * (argc+1));
    } else if(argv[0] == 0) {
        // Empty argv
        new_argv[0] = HxLinker;
        new_argv[1] = (char*)new_path;
        new_argv[2] = 0;
        new_path = HxLinker;
    } else {
        // Execute with linker
        new_argv[0] = HxLinker;
        new_argv[1] = "--argv0";
        new_argv[2] = argv[0];
        new_argv[3] = (char*)new_path;
        int i = 4;
        for(int y = 1; argv[y] != 0; y++) {
            new_argv[i] = argv[y];
            i += 1;
        }
        new_argv[i] = 0;
        new_path = HxLinker;
    }

    if(HxDebug) {
        eprintf("execve(\"%s\", [", new_path);
        eprintf("\"%s\"", new_argv[0]);
        for(char *const *cur = new_argv+1; *cur != 0; cur++) {
            eprintf(", \"%s\"", *cur);
        }
        eprintf("], %p)\n", envp);
    }

    return execve_real(new_path, new_argv, envp);
}

static int HxHandleProot(const char *path, char *const argv[], char *const envp[]) {
    int argc = HxCountArgv(argv);
    int envc = HxCountArgv(envp);

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
        return HxHandleShebang(buf, new_path, argv, envp);
    } else if(HxIsElf(buf, nr) && HxIsElf64(buf, nr)) {
        Elf64_Ehdr ehdr;
        memcpy(&ehdr, buf, sizeof(Elf64_Ehdr));

        int ret = HxHasInterp(fd, &ehdr);
        if(ret == -1) return -1;
        if(ret) {
            return HxHandleElf(&ehdr, new_path, argv, envp);
        }
    }

    if(!HxProot) {
        eprintf("HxProot variable is unset\n");
        errno = ENOENT;
        return -1;
    }

    return HxHandleProot(path, argv, envp);
}

int execv(const char *path, char *const argv[]) {
    return execve(path, argv, environ);
}

int execvpe(const char *file, char *const argv[], char *const envp[]) {
    const char *found = HxEnvPathFind(file);
    if(!found) {
        errno = ENOENT;
        return -1;
    }
    return execve(found, argv, envp);
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

    const char *found = HxEnvPathFind(file);
    if(!found) {
        errno = ENOENT;
        return -1;
    }

    return execve(found, argv, environ);
}

int execveat(int dirfd, const char *path, char *const argv[], char *const envp[], int flags) {
    eprintf("UNIMPLEMENTED SHIT! execveat\n"); abort();
}
