#include <stdint.h>
#include <asm/unistd.h>
#include <dlfcn.h>
#include <errno.h>
#include <linux/landlock.h>
#include <linux/futex.h>

struct HxRobustListHead {
    char nothing[0x20];
    struct {
        void *next;
        long futex_offset;
        void *list_op_pending;
    } head;
};

struct HxRobustList {
    void *prev, *next;
};

// b"land"
#define LANDLOCK_FD 1818324580

static long int (*syscall_real)(long int num, int64_t a0, int64_t a1, int64_t a2, int64_t a3, int64_t a4, int64_t a5);
long int syscall(long int num, int64_t a0, int64_t a1, int64_t a2, int64_t a3, int64_t a4, int64_t a5) {
    if(!syscall_real) syscall_real = dlsym(RTLD_NEXT, "syscall");

    switch(num) {
        // pam
        case __NR_keyctl:
            errno = ENOSYS;
            return -1;
        // pacman
        case __NR_landlock_create_ruleset:
            if(a3 == LANDLOCK_CREATE_RULESET_VERSION) {
                return 3;
            } else {
                return LANDLOCK_FD;
            }
        case __NR_landlock_add_rule:
        case __NR_landlock_restrict_self:
            return 0;
        // steam
        case __NR_get_robust_list:
            static struct HxRobustListHead head = {0};
            static struct HxRobustList next = {0};
            head.head.next = &next.next;
            head.head.futex_offset = -0x20;
            head.head.list_op_pending = (void*) 0LL;
            next.prev = &head.head;
            next.next = &head.head;
            void **head_ptr = (void**) a1;
            size_t *sizep = (size_t*) a2;
            *head_ptr = &head.head;
            *sizep = sizeof(struct robust_list_head);
            return 0;
    }

    return syscall_real(num, a0, a1, a2, a3, a4, a5);
}
