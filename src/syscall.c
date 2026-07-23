#include <stdint.h>
#include <asm/unistd.h>
#include <dlfcn.h>
#include <errno.h>
#include <linux/landlock.h>

// b"land"
#define LANDLOCK_FD 1818324580

static long int (*syscall_real)(long int num, int64_t a0, int64_t a1, int64_t a2, int64_t a3, int64_t a4, int64_t a5);
long int syscall(long int num, int64_t a0, int64_t a1, int64_t a2, int64_t a3, int64_t a4, int64_t a5) {
    if(!syscall_real) syscall_real = dlsym(RTLD_NEXT, "syscall");

    switch(num) {
        case __NR_keyctl:
            errno = ENOSYS;
            return -1;
        case __NR_landlock_create_ruleset:
            if(a3 == LANDLOCK_CREATE_RULESET_VERSION) {
                return 3;
            } else {
                return LANDLOCK_FD;
            }
        //,break;
        case __NR_landlock_add_rule:
        case __NR_landlock_restrict_self:
            return 0;
    }

    return syscall_real(num, a0, a1, a2, a3, a4, a5);
}
