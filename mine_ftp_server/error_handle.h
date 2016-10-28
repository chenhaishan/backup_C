#ifndef ERROR_HANDLE_H
#define ERROR_HANDLE_H

#include <errno.h>

#ifndef MAXLINE
#define MAXLINE 1024
#endif

extern void err_sys(const char *cause, ...);
/*
 * show system error msg and quit
*/
#define err_sys_q(cause, ...) \
    do { \
        err_sys(cause, ##__VA_ARGS__); \
        exit(EXIT_FAILURE);
    }while (0)

extern void err_msg(const char *cause, ...);
/*
 * show error msg and quit
*/
#define err_msg_q(cause, ...) \
    do { \
        err_msg(cause, ##__VA_ARGS__); \
        exit(EXIT_FAILURE); \
    }while (0)

extern void t_err_sys(int t_err_code, const char *cause, ...);



#endif // ERROR_HANDLE_H
