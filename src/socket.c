#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/un.h>
#include <linux/un.h>
#include <dlfcn.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "hxroot.h"

static int (*bind_real)(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    if(!bind_real) bind_real = dlsym(RTLD_NEXT, "bind");

    if(addr->sa_family == AF_UNIX) {
        struct sockaddr_un *uaddr = (struct sockaddr_un*) addr;

        // If not abstract socket
        if(uaddr->sun_path[0] != '\0') {
            struct sockaddr_un new_addr = {0};
            new_addr.sun_family = AF_UNIX;

            int len = HxL(uaddr->sun_path);

            if(len > UNIX_PATH_MAX) {
                errno = E2BIG;
                return -1;
            }

            if(len != 0) {
                char pathbuf[len];
                const char *new_path = HxExpandPath(pathbuf, uaddr->sun_path);
                strncpy(new_addr.sun_path, new_path, UNIX_PATH_MAX);

                return bind_real(sockfd, (struct sockaddr*) &new_addr, SUN_LEN(&new_addr));
            }
        }
    }

    return bind_real(sockfd, addr, addrlen);
}

static int (*connect_real)(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    if(!connect_real) connect_real = dlsym(RTLD_NEXT, "connect");

    if(addr->sa_family == AF_UNIX) {
        struct sockaddr_un *uaddr = (struct sockaddr_un*) addr;

        // If not abstract socket
        if(uaddr->sun_path[0] != '\0') {
            struct sockaddr_un new_addr = {0};
            new_addr.sun_family = AF_UNIX;

            int len = HxL(uaddr->sun_path);

            if(len > UNIX_PATH_MAX) {
                errno = E2BIG;
                return -1;
            }

            if(len != 0) {
                char pathbuf[len];
                const char *new_path = HxExpandPath(pathbuf, uaddr->sun_path);
                strncpy(new_addr.sun_path, new_path, UNIX_PATH_MAX);

                return connect_real(sockfd, (struct sockaddr*) &new_addr, SUN_LEN(&new_addr));
            }
        }
    }

    return connect_real(sockfd, addr, addrlen);
}

static ssize_t (*recvmsg_real)(int sockfd, struct msghdr *msg, int flags);
ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
    if(!recvmsg_real) recvmsg_real = dlsym(RTLD_NEXT, "recvmsg");

    ssize_t ret = recvmsg_real(sockfd, msg, flags);
    if(ret == -1) return -1;

    if(msg->msg_controllen != 0) {
        for(struct cmsghdr *cmsg = CMSG_FIRSTHDR(msg); cmsg != NULL; cmsg = CMSG_NXTHDR((struct msghdr*) msg, cmsg)) {
            if(cmsg->cmsg_type == SCM_CREDENTIALS) {
                struct ucred *ucred = (struct ucred*) CMSG_DATA(cmsg);
                ucred->uid = getuid();
                ucred->gid = getgid();
            }
        }
    }

    return ret;
}

static uid_t (*getuid_real)(void);
static gid_t (*getgid_real)(void);
static ssize_t (*sendmsg_real)(int sockfd, const struct msghdr *msg, int flags);
ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags) {
    if(!sendmsg_real) sendmsg_real = dlsym(RTLD_NEXT, "sendmsg");
    if(!getuid_real) getuid_real = dlsym(RTLD_NEXT, "getuid");
    if(!getgid_real) getgid_real = dlsym(RTLD_NEXT, "getgid");

    if(msg->msg_controllen != 0) {
        for(struct cmsghdr *cmsg = CMSG_FIRSTHDR(msg); cmsg != NULL; cmsg = CMSG_NXTHDR((struct msghdr*) msg, cmsg)) {
            if(cmsg->cmsg_type == SCM_CREDENTIALS) {
                struct ucred *ucred = (struct ucred*) CMSG_DATA(cmsg);
                ucred->uid = getuid_real();
                ucred->gid = getgid_real();
            }
        }
    }

    return sendmsg_real(sockfd, msg, flags);
}

static int (*getsockopt_real)(int sockfd, int level, int optname, void *optval, socklen_t *restrict optlen);
int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *restrict optlen) {
    if(!getsockopt_real) getsockopt_real = dlsym(RTLD_NEXT, "getsockopt");

    int ret = getsockopt_real(sockfd, level, optname, optval, optlen);

    if(level == SOL_SOCKET && optname == SO_PEERCRED) {
        struct ucred *ucred = (struct ucred*) optval;
        ucred->uid = getuid();
        ucred->gid = getgid();
    }

    return ret;
}
