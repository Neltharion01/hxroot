#define _GNU_SOURCE
#include <stdatomic.h>
#include <stddef.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <stdarg.h>
#include <limits.h>

#include "hxroot.h"

#ifndef SEMMSL
#define SEMMSL 32000
#endif
#ifndef SEMOPM
#define SEMOPM 500
#endif

#define GUARD __attribute__((cleanup(FutexUnlock))) guard_t
#define SHARED_GUARD __attribute__((cleanup(FutexUnlockShared))) guard_t

typedef atomic_uint futex_t;
typedef futex_t *guard_t;

enum HxFutexState {
    UNLOCKED, LOCKED, CONTENDED
};

static void futex_wait(futex_t *mtx, int flag, unsigned int expected) {
    int ret = syscall(SYS_futex, mtx, FUTEX_WAIT|flag, expected, NULL);
    if(ret == -1) {
        if(errno != EAGAIN && errno != EINTR) {
            eprintf("futex_wait failed: %s\n", strerror(errno));
            abort();
        }
    }
}

static void futex_wake(futex_t *mtx, int flag) {
    int ret = syscall(SYS_futex, mtx, FUTEX_WAKE|flag, 1);
    if(ret == -1) {
        eprintf("futex_wake failed: %s\n", strerror(errno));
        abort();
    }
}

static guard_t FutexLockCore(futex_t *mtx, int flag) {
    // Fast uncontended case
    unsigned int val = LOCKED;
    if(atomic_compare_exchange_strong(mtx, &val, UNLOCKED)) {
        // Success
        return mtx;
    }

    // Slow path
    for(;;) {
        if(atomic_exchange(mtx, CONTENDED) == UNLOCKED) {
            // Success
            return mtx;
        }
        futex_wait(mtx, flag, CONTENDED);
    }
}

static guard_t FutexLock(futex_t *mtx) {
    return FutexLockCore(mtx, FUTEX_PRIVATE_FLAG);
}

static guard_t FutexLockShared(futex_t *mtx) {
    return FutexLockCore(mtx, 0);
}

static void FutexUnlockCore(guard_t *guard, int flag) {
    unsigned int state = atomic_exchange(*guard, UNLOCKED);
    switch(state) {
        case UNLOCKED:
            eprintf("attempt to unlock an unlocked mutex\n");
            abort();
        case LOCKED:
            return;
        case CONTENDED:
            futex_wake(*guard, flag);
    }
}

static void FutexUnlock(guard_t *guard) {
    FutexUnlockCore(guard, FUTEX_PRIVATE_FLAG);
}

static void FutexUnlockShared(guard_t *guard) {
    FutexUnlockCore(guard, 0);
}

static int futex_wait_timed(futex_t *mtx, int flag, unsigned int expected, const struct timespec *timeout) {
    int ret = syscall(SYS_futex, mtx, FUTEX_WAIT|flag, expected, NULL);
    if(ret == -1) {
        if(errno == ETIMEDOUT || errno == EINTR) return -1;
        if(errno != EAGAIN) {
            eprintf("futex_wait failed: %s\n", strerror(errno));
            abort();
        }
    }
    return 0;
}

static int CondWaitShared(futex_t *cond, guard_t guard, const struct timespec *timeout) {
    unsigned int val = atomic_load(cond);

    FutexUnlockShared(&guard);

    int ret = futex_wait_timed(cond, 0, val, timeout);
    int saved_errno = errno;

    FutexLockShared(guard);

    errno = saved_errno;
    if(ret == -1) {
        return -1;
    } else {
        return 0;
    }
}

static void futex_wake_all(futex_t *mtx, int flag) {
    int ret = syscall(SYS_futex, mtx, FUTEX_WAKE|flag, INT_MAX);
    if(ret == -1) {
        eprintf("futex_wake failed: %s\n", strerror(errno));
        abort();
    }
}

static void CondSignalAllShared(futex_t *cond) {
    atomic_fetch_add(cond, 1);
    futex_wake_all(cond, 0);
}

struct HxSem {
    int value;
    int ncnt;
    int zcnt;
    int pid;
};

struct HxSemSet {
    futex_t lock;
    futex_t cond;
    int nsems;
    struct HxSem sems[];
};

#define SET_SIZE(n) (sizeof(futex_t) * 2 + sizeof(int) + sizeof(struct HxSem) * n)

// Array map
struct HxSetMap {
    void **sets;
    int len;
    futex_t lock;
};
static struct HxSetMap SetMap = {
    .sets = NULL,
    .len = 0,
    .lock = UNLOCKED,
};

static int AddSetToMap(void *addr) {
    GUARD guard = FutexLock(&SetMap.lock);

    if(SetMap.len == 0) {
        void *ret = calloc(128, sizeof(void*));
        if(!ret) return -1;
        SetMap.sets = ret;
        SetMap.len = 128;
    }

    int idx = 0;
    while(SetMap.sets[idx] != NULL && idx < SetMap.len) idx += 1;

    if(idx == SetMap.len) {
        void *ret = reallocarray(SetMap.sets, SetMap.len*2, sizeof(void*));
        if(!ret) return -1;
        memset(ret + SetMap.len, 0, sizeof(void*)*SetMap.len);
        SetMap.sets = ret;
        SetMap.len *= 2;
    }

    SetMap.sets[idx] = addr;
    return idx;
}

static void *RetrieveSetFromMap(int idx) {
    GUARD guard = FutexLock(&SetMap.lock);
    if(idx >= SetMap.len) return NULL;
    return SetMap.sets[idx];
}

int semget(key_t key, int nsem, int flags) {
    // Check nsem
    if(flags & IPC_CREAT && (nsem < 0 || nsem > SEMMSL)) {
        errno = EINVAL;
        return -1;
    }

    if(key == IPC_PRIVATE) {
        // Not using malloc because forked processes should inherit the semaphore (which MAP_SHARED does support)
        void *addr = mmap(NULL, SET_SIZE(nsem), PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, -1, 0);
        if(addr == MAP_FAILED) return -1;
        return AddSetToMap(addr);
    }

    int mode = flags & 0x1ff;
    int oflag = 0;
    if(flags & IPC_CREAT) oflag |= O_CREAT;
    if(flags & IPC_EXCL) oflag |= O_EXCL;

    AUTO_FREE_CHAR char *name = NULL;
    if(asprintf(&name, "hxsem.%x", key) == -1) return -1;

    int fd = shm_open(name, O_RDWR|oflag, mode);
    if(fd == -1) return -1;

    if(ftruncate(fd, SET_SIZE(nsem)) == -1) return -1;

    void *addr = mmap(NULL, SET_SIZE(nsem), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(addr == MAP_FAILED) return -1;

    return AddSetToMap(addr);
}

union semun {
    int val;				// value for SETVAL
    struct semid_ds *buf;		// buffer for IPC_STAT & IPC_SET
    unsigned short int *array;		// array for GETALL & SETALL
    struct seminfo *__buf;		// buffer for IPC_INFO
};

static int semctl_getncnt(int id, int num) {
    struct HxSemSet *set = RetrieveSetFromMap(id);
    if(!set || num >= set->nsems) {
        errno = EINVAL;
        return -1;
    }

    SHARED_GUARD guard = FutexLockShared(&set->lock);
    return set->sems[num].ncnt;
}

static int semctl_getzcnt(int id, int num) {
    struct HxSemSet *set = RetrieveSetFromMap(id);
    if(!set || num >= set->nsems) {
        errno = EINVAL;
        return -1;
    }

    SHARED_GUARD guard = FutexLockShared(&set->lock);
    return set->sems[num].zcnt;
}

static int semctl_setval(int id, int num, int val) {
    struct HxSemSet *set = RetrieveSetFromMap(id);
    if(!set || num >= set->nsems) {
        errno = EINVAL;
        return -1;
    }

    SHARED_GUARD guard = FutexLockShared(&set->lock);
    set->sems[num].value = val;
    CondSignalAllShared(&set->cond);

    return 0;
}

int semctl(int id, int num, int op, ...) {
    va_list ap;
    va_start(ap, op);
    union semun arg = va_arg(ap, union semun);
    va_end(ap);

    if(num < 0 || num > SEMMSL) {
        errno = EINVAL;
        return -1;
    }

    switch(op) {
        case GETNCNT: return semctl_getncnt(id, num);
        case GETZCNT: return semctl_getzcnt(id, num);
        case SETVAL: return semctl_setval(id, num, arg.val);
        case IPC_STAT:
        case IPC_SET:
        case IPC_RMID:
        case IPC_INFO:
        case SEM_INFO:
        case SEM_STAT:
        case SEM_STAT_ANY:
        case GETALL:
        case GETPID:
        case GETVAL:
        case SETALL:
            eprintf("unimplemented semctl operation %d\n", op);
            abort();
        default:
            errno = EINVAL;
            return -1;
    }
}

struct HxOpInfo {
    int semnum;
    int op;
};

enum HxOp {
    NOTHING = 0, NCNT = 1, ZCNT = 2
};

int semtimedop(int id, struct sembuf* ops, size_t op_count, const struct timespec* ts) {
    struct HxSemSet *set = RetrieveSetFromMap(id);
    if(!set) {
        errno = EINVAL;
        return -1;
    }

    SHARED_GUARD guard = FutexLockShared(&set->lock);
    for(;;) {
        struct HxOpInfo undo_ops[op_count];
        int undo_idx = 0;

        bool can_process = true;
        for(int i = 0; i < op_count; i++) {
            struct sembuf *op = &ops[i];
            struct HxSem *sem = &set->sems[op->sem_num];

            if(op->sem_num >= set->nsems) {
                errno = EFBIG;
                return -1;
            }

            if(op->sem_op == 0) {
                // Wait for zero operation
                if(sem->value != 0) {
                    can_process = false;
                    sem->zcnt += 1;
                    undo_ops[undo_idx].semnum = op->sem_num;
                    undo_ops[undo_idx].op = ZCNT;
                    undo_idx += 1;
                    if(op->sem_flg & IPC_NOWAIT) {
                        errno = EAGAIN;
                        return -1;
                    }
                }
            } else if(op->sem_op < 0) {
                // Down operation
                if(op->sem_op + sem->value < 0) {
                    can_process = false;
                    sem->ncnt += 1;
                    undo_ops[undo_idx].semnum = op->sem_num;
                    undo_ops[undo_idx].op = NCNT;
                    undo_idx += 1;
                    if(op->sem_flg & IPC_NOWAIT) {
                        errno = EAGAIN;
                        return -1;
                    }
                }
            }
        }

        if(!can_process) {
            int waitresult = CondWaitShared(&set->cond, guard, ts);
            for(int i = 0; i <= undo_idx; i++) {
                struct HxOpInfo *undo = &undo_ops[i];
                if(undo->op == ZCNT) {
                    set->sems[undo->semnum].zcnt -= 1;
                } else if(undo->op == NCNT) {
                    set->sems[undo->semnum].ncnt -= 1;
                }
            }
            if(waitresult == -1) return -1;
            continue;
        }

        for(int i = 0; i < op_count; i++) {
            struct sembuf *op = &ops[i];
            struct HxSem *sem = &set->sems[op->sem_num];
            sem->value += op->sem_op;
            // Up operation
            if(op->sem_op > 0) CondSignalAllShared(&set->cond);
        }

        return 0;
    }
}

int semop(int id, struct sembuf* ops, size_t op_count) {
    return semtimedop(id, ops, op_count, NULL);
}
