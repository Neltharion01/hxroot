#define _GNU_SOURCE
#include <spawn.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>

#include "util.h"

// Due to ours execve function complexity, we have to reimplement these amazing apis using execve

#ifdef __GLIBC__
struct __spawn_action {
    enum {
        spawn_do_close,
        spawn_do_dup2,
        spawn_do_open
    } tag;

    union {
        struct {
            int fd;
        } close_action;

        struct {
            int fd;
            int newfd;
        } dup2_action;

        struct {
            int fd;
            char *path;
            int oflag;
            mode_t mode;
          } open_action;
    } action;
};
#endif

static int HxProcessFileActions(const posix_spawn_file_actions_t *restrict file_actions) {
    for(int i = 0; i < file_actions->__used; i++) {
        struct __spawn_action *cur = &file_actions->__actions[i];
        switch(cur->tag) {
            case spawn_do_close:
                close(cur->action.close_action.fd);
            break;
            case spawn_do_dup2:
                if(dup2(cur->action.dup2_action.fd, cur->action.dup2_action.newfd) == -1) {
                    return -1;
                }
            break;
            case spawn_do_open:
                int ret = open(cur->action.open_action.path, cur->action.open_action.oflag, cur->action.open_action.mode);
                if(ret == -1) return -1;
                if (ret != cur->action.open_action.fd) {
                    if(dup2(ret, cur->action.open_action.fd) == -1) return -1;
                    if(close(ret) == -1) return -1;
                }
            break;
            default:
                eprintf("Unsupported posix_spawn action: %d\n", cur->tag); abort();
            break;
        }
    }

    return 0;
}

static int HxProcessSpawnAttrs(const posix_spawnattr_t *restrict attrp) {
    // not implemented
    return 0;
}

int posix_spawn(pid_t *restrict pid, const char *restrict path,
                       const posix_spawn_file_actions_t *restrict file_actions,
                       const posix_spawnattr_t *restrict attrp,
                       char *const argv[restrict],
                       char *const envp[restrict])
{
    pid_t ret = fork();
    if(ret == -1) {
        return errno;
    } else if(ret == 0) {
        // child
        if(HxProcessFileActions(file_actions) == -1) exit(127);
        if(HxProcessSpawnAttrs(attrp) == -1) exit(127);
        execve(path, argv, envp); exit(127);
    } else {
        // parent
        *pid = ret;
        return 0;
    }
}

int posix_spawnp(pid_t *restrict pid, const char *restrict file,
                       const posix_spawn_file_actions_t *restrict file_actions,
                       const posix_spawnattr_t *restrict attrp,
                       char *const argv[restrict],
                       char *const envp[restrict])
{
    pid_t ret = fork();
    if(ret == -1) {
        return errno;
    } else if(ret == 0) {
        // child
        if(HxProcessFileActions(file_actions) == -1) exit(127);
        if(HxProcessSpawnAttrs(attrp) == -1) exit(127);
        execvpe(file, argv, envp); exit(127);
    } else {
        // parent
        *pid = ret;
        return 0;
    }
}
