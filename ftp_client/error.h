#ifndef _ERROR_H

#include <errno.h>
#include <stdarg.h>

#ifndef MAXLINE
#define MAXLINE	1024
#endif

extern void std_err_sys(const char *cause, ...);
extern void std_err_sys_q(const char *cause, ...);
extern void std_err_msg(const char *cause, ...);
extern void std_err_msg_q(const char *cause, ...);
extern void sock_err_sys(const char *cause, ...);
extern void sock_err_sys_q(const char *cause, ...);
#define err_msg			std_err_msg
#define err_msg_q		std_err_msg_q
#define sock_err_msg	std_err_msg
#define sock_err_msg_q	std_err_msg_q
extern void win_err_sys(const char *cause, ...);
extern void win_err_sys_q(const char *cause, ...);
#define win_err_msg		std_err_msg
#define win_err_msg_q	std_err_msg_q
extern void err_handle(int errnoflg, const char *fmt, va_list ap);
#endif
