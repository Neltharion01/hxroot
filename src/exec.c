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

static size_t arraylen(char *const arr[]) {
    size_t count = 0;
    for(char *const *cur = arr; *cur != 0; cur++) count += 1;
    return count;
}

int execveat(int dirfd, const char *path, char *const argv[], char *const envp[], int flags) {
    eprintf("UNIMPLEMENTED SHIT! execveat\n"); abort();
}

static int (*execve_real)(const char *path, char *const argv[], char *const envp[]);
int execve(const char *path, char *const argv[], char *const envp[]) {
    if(!execve_real) execve_real = dlsym(RTLD_NEXT, "execve");
    HxInit();

    size_t argc = arraylen(argv);

    char new_path[PATH_MAX];
    if(HxRoot) HxExpandPath_r(new_path, path);
    else strncpy(new_path, path, PATH_MAX);

    AUTO_CLOSE int fd = open(path, O_RDONLY);
    if(fd == -1) return -1;

    char buf[4096];
    ssize_t nr = read(fd, buf, 4096-1);
    if(nr == -1) return -1;

    // Shebang
    if(nr >= 2 && buf[0] == '#' && buf[1] == '!') {
        char *interp = buf + 2;

        // 1) Find newline
        char *newline = memchr(buf, '\n', nr);
        if(!newline) newline = buf + nr;
        *newline = '\0';

        char *interp_name = strrchr(interp, '/');
        if(!interp_name) interp_name = interp;
        else interp_name += 1;

        // 2) Find space
        char *space = memchr(buf, ' ', newline - buf);
        // Left branch needs interp_name, path, ..argv[1:] and '\0' (argc+2)
        // Right branch needs interp_name, arg, path, argv[1:] and '\0' (argc+3)
        char *new_argv[argc+3];
        if(!space) {
            // Don't have arg
            new_argv[0] = interp_name;
            new_argv[1] = (char*)new_path;
            // Original argv[0] is script's name, we skip it
            memcpy(new_argv+2, argv+1, sizeof(char*)*argc);
        } else {
            // Has arg
            *space = '\0';
            char *arg = space + 1;

            new_argv[0] = interp_name;
            new_argv[1] = arg;
            new_argv[2] = (char*)new_path;
            // Same
            memcpy(new_argv+3, argv+1, sizeof(char*)*argc);
        }

        return execve(interp, new_argv, envp);
    }

    // Elf
    if(nr >= EI_NIDENT && buf[0] == 0x7f && buf[1] == 'E' && buf[2] == 'L' && buf[3] == 'F') {
        if(buf[EI_CLASS] == ELFCLASS64 && nr >= (ssize_t)sizeof(Elf64_Ehdr)) {
            // Elf64
            Elf64_Ehdr ehdr;
            memcpy(&ehdr, buf, sizeof(Elf64_Ehdr));

            if(ehdr.e_shnum * sizeof(Elf64_Shdr) > 4096) {
                // We aren't going to allocate that much
                errno = E2BIG;
                return -1;
            }
            Elf64_Shdr sections[ehdr.e_shnum];

            off_t off = lseek(fd, ehdr.e_shoff, SEEK_SET);
            if(off == -1) return -1;
            nr = read(fd, sections, sizeof(Elf64_Shdr)*ehdr.e_shnum);
            if(nr == -1) return -1;
            if(nr != (ssize_t)sizeof(Elf64_Shdr)*ehdr.e_shnum) { errno = ENODATA; return -1; }

            Elf64_Shdr *strtab = &sections[ehdr.e_shstrndx];
            if(strtab->sh_size > 4096) {
                errno = E2BIG;
                return -1;
            }
            char strtab_s[strtab->sh_size];

            off = lseek(fd, strtab->sh_offset, SEEK_SET);
            if(off == -1) return -1;
            nr = read(fd, strtab_s, strtab->sh_size);
            if(nr == -1) return -1;
            if(nr != (ssize_t)strtab->sh_size) { errno = ENODATA; return 1; }

            int interp_off = -1;
            for(int i = 0; i < ehdr.e_shnum; i++) {
                char *name = strtab_s + sections[i].sh_name;
                if(strcmp(name, ".interp") == 0) {
                    // Finally found it
                    interp_off = i;
                    break;
                }
            }

            if(interp_off == -1) {
                // No .interp == statically linked
                if(HxDebug) eprintf("Static binary detected! Launching proot\n");
                if(!HxProot) {
                    eprintf("HxProot is unset\n");
                    abort();
                }

                // LD_PRELOAD= proot -r $HxRoot -w $PWD -b $HxBinds -i $HxUid:$HxGid <new_path> <argv[1:]> \0

                // 1) Unset LD_PRELOAD
                size_t envc = 0;
                for(int i = 0; envp[i] != 0; i++) envc += 1;

                char *new_envp[envc+1];
                memset(new_envp, 0, sizeof(char*) * (envc+1));
                int new_envp_idx = 0;
                for(int i = 0; envp[i] != 0; i++) {
                    if(strncmp("LD_PRELOAD=", envp[i], 11) != 0) {
                        new_envp[new_envp_idx] = envp[i];
                        new_envp_idx += 1;
                    }
                }

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
                for(size_t y = 1; y <= argc; y++) {
                    new_argv[i] = argv[y];
                    i += 1;
                }

                return execve_real(HxProot, new_argv, new_envp);
            } else {
                // argc == 0: ld-linux-aarch64.so.1 <new_path> \0
                // argc >= 1: ld-linux-aarch64.so.1 --argv0 <argv[0]> <new_path> <argv[1:]> \0
                char *new_argv[argc+4];
                if(!HxLinker) {
                    memcpy(new_argv, argv, sizeof(char*) * (argc+1));
                } else if(argv[0] == 0) {
                    new_argv[0] = HxLinker;
                    new_argv[1] = new_path;
                    new_argv[2] = 0;
                } else {
                    new_argv[0] = HxLinker;
                    new_argv[1] = "--argv0";
                    new_argv[2] = argv[0];
                    new_argv[3] = new_path;
                    int i = 4;
                    for(char *const *cur = argv+1; *cur != 0; cur++) {
                        new_argv[i] = *cur;
                        i += 1;
                    }
                    new_argv[i] = 0;
                }

                if(HxDebug) {
                    eprintf("execve(\"%s\" -> \"%s\", [", path, new_path);
                    eprintf("\"%s\"", new_argv[0]);
                    for(char *const *cur = new_argv+1; *cur != 0; cur++) {
                        eprintf(", \"%s\"", *cur);
                    }
                    eprintf("], %p)\n", envp);
                }

                return execve_real(HxLinker, new_argv, envp);
            }
        } else if(buf[EI_CLASS] == ELFCLASS32 && nr >= (ssize_t)sizeof(Elf32_Ehdr)) {
            // Elf32
            eprintf("Unsupported 32-bit binary!\n"); abort();
        }
    }

    // Not elf and not shebang
    return execve_real(new_path, argv, envp);
}

int execv(const char *path, char *const argv[]) {
    HxInit();

    if(HxDebug) eprintf("execv -> execve\n");
    return execve(path, argv, environ);
}

int execvpe(const char *file, char *const argv[], char *const envp[]) {
    HxInit();

    if(HxDebug) eprintf("execvpe -> execve\n");
    const char *new_file = file;
    if(HxRoot) new_file = HxEnvPathFind(file);
    return execve(new_file, argv, envp);
}

int execvp(const char *file, char *const argv[]) {
    HxInit();

    if(HxDebug) eprintf("execvp -> execvpe\n");
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

    const char *new_file = file;
    if(HxRoot) new_file = HxEnvPathFind(file);

    return execve(new_file, argv, environ);
}
