#include "error_handle.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>

/*
 *err_handle: error handle funciton
 * @errnoflg: if none zero will show sys error, otherwise wo'nt
 * @thread_err_code: error code in thread
 * @fmt: error string format wants printing
 * @ap: handle arguments
*/

static void err_handle(int errnoflg, int thgread_err_code,
                       const char *fmt, va_list ap){
    int errno_save, n;
    char buf[MAXLINE];

    vsnprintf(buf, sizeof(buf), fmt, ap);
    n = strlen(buf);
    /*show system error msg*/
    if(1 == errnoflg){
        errno_save = errno;
        snprintf(buf+n, sizeof(buf)-n,
                 ": %s", strerror(errno_save));
    }
    else if(2 == errnoflg){
        snprintf(buf+n, sizeof(buf)-n,
                 ": %s", strerror(thread_err_code));
    }
    strcat(buf, "\n");
    /*output the final error msg*/
    fflush(stdout);/*in case stdout and stderr are the same*/
    my_lock_mutex_wait();
    fputs(buf, stderr);
    my_lock_mutex_release();
    fflush(stderr);
    return ;

}/*end of err_handle*/

/*
 * print system error msg
*/
void err_sys(const char *cause, ...){
    va_list ap;
    va_start(ap, cause);
    err_handle(1, 0, cause, ap);
    va_end(ap);

}/*end of err_sys*/

/*
 * err_msg: print normal error msg
*/
void err_msg(const char *cause, ...){
    va_list ap;
    va_start(ap, cause);
    err_handle(0, 0, cause, ap);
    va_end(ap);
}/*end of err_msg*/

/*
 * thread_err_sys: thread print system error msg, then quit
*/
void thread_err_sys(int thread_err_code, const char *cause, ...){
    va_list ap;
    va_start(ap, cause);
    err_handle(2, thread_err_code, cause, ap);
    va_end(ap);
    exit(EXIT_FAILURE);
}/*end of thread_err_sys*/
