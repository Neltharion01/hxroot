#include <stdint.h>
#include <asm/unistd.h>
#include <dlfcn.h>
#include <errno.h>

static long int (*syscall_real)(long int num, int64_t a0, int64_t a1, int64_t a2, int64_t a3, int64_t a4, int64_t a5);
long int syscall(long int num, int64_t a0, int64_t a1, int64_t a2, int64_t a3, int64_t a4, int64_t a5) {
    if(!syscall_real) syscall_real = dlsym(RTLD_NEXT, "syscall");

    if(num == __NR_keyctl) {
        errno = ENOSYS;
        return -1;
    }

    return syscall_real(num, a0, a1, a2, a3, a4, a5);
}
