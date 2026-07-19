#include <sys/prctl.h>
#include <stdarg.h>
#include <dlfcn.h>

static int (*prctl_real)(int op, ...);
int prctl(int op, ...) {
    if(!prctl_real) prctl_real = dlsym(RTLD_NEXT, "prctl");

    if(op == PR_SET_DUMPABLE) return 0;

    unsigned long x[4];
    int i;
    va_list ap;
    va_start(ap, op);
    for (i=0; i<4; i++) x[i] = va_arg(ap, unsigned long);
    va_end(ap);

    return prctl_real(op, x[0], x[1], x[2], x[3]);
}
