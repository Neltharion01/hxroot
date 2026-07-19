#include <dlfcn.h>
#include <stdio.h>

// b"audt"
// Large enough not to be a real fd and small enough to fit in an int
#define AUDIT_PLACEHOLDER 1635083380

int audit_open(void) {
    return AUDIT_PLACEHOLDER;
}

int audit_log_acct_message(int audit_fd, int type, const char *pgname,
        const char *op, const char *name, unsigned int id,
        const char *host, const char *addr, const char *tty, int result)
{
    return 1;
}

int audit_log_user_command(int audit_fd, int type, const char *command,
        const char *tty, int result)
{
    return 1;
}

static int (*fcntl_real)(int fd, int op, unsigned long arg);
int fcntl(int fd, int op, unsigned long arg) {
    if(!fcntl_real) fcntl_real = dlsym(RTLD_NEXT, "fcntl");
    if(fd == AUDIT_PLACEHOLDER) return 0;
    return fcntl_real(fd, op, arg);
}
