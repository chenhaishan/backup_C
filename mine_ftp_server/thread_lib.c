#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include "socket_rw_wrapper.h"
#include "thread_lib.h"
#include "error_handle.h"

//rl means readline
static pthread_key_t rl_key;
static ptread_once_t rl_once = PTHREAD_ONCE_INIT;
static pthread_mutex_t *mptr;


/*wrapper for pthread_create*/
void Pthread_create(pthread_t *tid, const pthread_attr_t *attr, void *(*func)(void *), void *arg){
    int n;
    if(0 == (n = pthread_create(tid, attr, func, arg)))
        return;
    errno = n;
    err_sys_q("pthread_create error");
}

/*wrapper for pthread_join*/
void Pthread_join(pthread_t tid, void **status){
    int n;
    if(0 == (pthread_join(tid, status)))
        return;
    errno = n;
    err_sys_q("pthread_join error");
}

/*wrapper for pthread_detach*/
void Pthread_detach(pthread_t tid){
    int n;
    if(0 == (n = pthread_detach(tid)))
        return;
    errno = n;
    err_sys_q("pthread_detach error");
}


/*wrapper for pthread_kill*/
void Pthread_kill(pthread_t tid, int signo){
    int n;
    if(0 == (n = pthread_kill(tid, signo)))
        return ;
    errno = n;
    err_sys_q("pthread_kill error");
}

/*wrapper for pthread_mutexattr_init*/

void Pthread_mutexattr_init(pthread_mutexattr_t *attr){
    int n;
    if(0 == (n = pthread_mutexattr_init(attr)))
        return;
    errno = n;
    err_sys_q("pthread_mutexattr_init error");
}

/*wrapper for pthread_mutexattr_setpshared*/
void Pthread_mutexattr_setpshared(pthread_mutexattr_t *attr, int flag){
    int n;
    if(0 == (n = pthread_mutexattr_setpshared(attr, flag)))
        return;
    errno = n;
    err_sys_q("pthread_mutexattr_setpshared error");
}

/*wrapper for pthread_mutex_init*/
void Pthread_mutex_init(pthread_mutex_t *mptr, pthread_mutexattr_t *attr){
    int n;
    if(0 == (n = pthread_mutex_init(mptr, attr)))
        return ;
    errno = n;
    err_sys_q("pthread_mutex_init error");
}

/*wrapper for pthread_mutex_lock*/
void Pthread_mutex_lock(pthread_mutex_t *mptr){
    int n;
    if(0 == (n = pthread_mutex_lock(mptr)))
        return;
    errno = n;
    err_sys_q("pthread_mutex_lock error");
}


/*wrapper for pthread_mutex_unlock*/
void Pthread_mutex_unlock(pthread_mutex_t *mptr){
    int n;
    if(0 == (n = pthread_mutex_unlock(mptr)))
        return;
    errno = n;
    err_sys_q("pthread_mutex_unlock error");
}

/*wrapper for pthread_mutex_destory*/
void Pthread_mutex_destory(pthread_mutex_t *mptr){
    int n;
    if(0 == (n = pthread_mutex_destory(mptr)))
        return;
    errno = n;
    err_sys_q("pthread_mutex_destory error");
}


/*wrapper for pthread_key_create*/
void Pthread_key_create(pthread_key_t *key, void (*func)(void *)){
    int n;
    if(0 == (n = pthread_key_create(key, func)))
        return;
    errno = n;
    err_sys_q("pthread_key_create error");
}

/*wrapper for pthread_once*/
void Pthread_once(pthread_once_t *ptr, void (*func)(void))
{
    int		n;

    if (0 == (n = pthread_once(ptr, func)))
        return;
    errno = n;
    err_sys_q("pthread_once error");
}

/*wrapper for pthread_setspecific*/
void Pthread_setspecific(pthread_key_t key, const void *value)
{
    int		n;

    if (0 == (n = pthread_setspecific(key, value)))
        return;
    errno = n;
    err_sys_q("pthread_setspecific error");
}

/*free readline buf*/
static void readline_destructor(void *ptr)
{
    free(ptr);
}

/**/
static void readline_once(void)
{
    Pthread_key_create(&rl_key, readline_destructor);
}

typedef struct {
    int		rl_cnt;
    char	*rl_bufptr;
    char	rl_buf[MAXLINE];
} Rline;

static ssize_t my_read(Rline *tsd, int fd, char *ptr)
{
    if (tsd->rl_cnt <= 0) {
again:
        if ((tsd->rl_cnt = read(fd, tsd->rl_buf, sizeof(tsd->rl_buf))) < 0) {
            if (EINTR == errno)
                goto again;
            return -1;
        } else if (0 == tsd->rl_cnt)
            return 0;
        tsd->rl_bufptr = tsd->rl_buf;
    }
    --tsd->rl_cnt;
    *ptr = *tsd->rl_bufptr++;
    return 1;
}

ssize_t readline_r(int fd, void *vptr, size_t maxlen)
{
    ssize_t	n, rc;
    char	c, *ptr;
    Rline	*tsd;

    Pthread_once(&rl_once, readline_once);
    if (NULL == (tsd = pthread_getspecific(rl_key))) {
        tsd = Calloc(1, sizeof(Rline));
        Pthread_setspecific(rl_key, tsd);
    }

    ptr = vptr;
    for (n = 1; n < maxlen; ++n) {
        if (1 == (rc = my_read(tsd, fd, &c))) {
            *ptr++ = c;
            if ('\n' == c)
                break;
        } else if (0 == rc) {
            if (1 == n)
                return 0;
            else
                break;
        } else
            return -1;
    }
    *ptr = '\0';
    return n;
}

ssize_t Readline_r(int fd, void *vptr, size_t maxlen)
{
    ssize_t	n;

    if (-1 == (n = readline_r(fd, vptr, maxlen)))
        err_sys_q("readline_r error");
    return n;
}

void my_lock_mutex_init(void)
{
    int		fd;
    pthread_mutexattr_t	mattr;

    fd = Open("/dev/zero", O_RDWR, 0);
    mptr = Mmap(0, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE,
            MAP_SHARED, fd, 0);
    Close(fd);

    Pthread_mutexattr_init(&mattr);
    Pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    Pthread_mutex_init(mptr, &mattr);
}

void my_lock_mutex_wait(void)
{
    Pthread_mutex_lock(mptr);
}

void my_lock_mutex_release(void)
{
    Pthread_mutex_unlock(mptr);
}


/*
 * semaphore
*/
void Sem_init(sem_t *sem, int pshared, unsigned value)
{
    if (-1 == sem_init(sem, pshared, value))
        err_sys_q("sem_init error");
}

void Sem_trywait(sem_t *sem)
{
    if (-1 == sem_trywait(sem))
        err_sys_q("sem_trywait error");
}

void Sem_wait(sem_t *sem)
{
    if (-1 == sem_wait(sem))
        err_sys_q("sem_wait error");
}

void Sem_post(sem_t *sem)
{
    if (-1 == sem_post(sem))
        err_sys_q("sem_post error");
}

void Sem_destroy(sem_t *sem)
{
    if (-1 == sem_destroy(sem))
        err_sys_q("sem_destroy error");
}





























