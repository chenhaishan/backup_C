#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <Winsock2.h>
#include "myconio.h"
#include "error.h"

/*
 * std_err_sys —— 输出错误信息（包括系统的错误信息）
 */
void std_err_sys(const char *cause, ...)
{
	va_list ap;

	va_start(ap, cause);
	err_handle(1, cause, ap);
	va_end(ap);
	return;
} /* end std_err_sys */

/*
 * std_err_sys_q —— 输出错误信息（包括系统的错误信息），并退出
 */
void std_err_sys_q(const char *cause, ...)
{
	va_list ap;

	va_start(ap, cause);
	err_handle(1, cause, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
} /* end std_err_sys_q */

/*
 * std_err_msg —— 输出错误信息
 */
void std_err_msg(const char *cause, ...)
{
	va_list ap;

	va_start(ap, cause);
	err_handle(0, cause, ap);
	va_end(ap);
	return;
}  /* end std_err_msg */

/*
 * std_err_msg_q —— 输出错误信息，并退出
 */
void std_err_msg_q(const char *cause, ...)
{
	va_list ap;

	va_start(ap, cause);
	err_handle(0, cause, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}  /* end std_err_msg_q */

/*
 * sock_err_sys —— 输出winsock的错误信息（包括系统的错误信息）
 */
void sock_err_sys(const char *cause, ...)
{
	va_list ap;

	va_start(ap, cause);
	err_handle(2, cause, ap);
	va_end(ap);
	return;
}  /* end sock_err_sys */

/*
 * sock_err_sys_q —— 输出winsock的错误信息（包括系统的错误信息），并退出
 */
void sock_err_sys_q(const char *cause, ...)
{
	va_list ap;

	va_start(ap, cause);
	err_handle(2, cause, ap);
	va_end(ap);
	WSACleanup();
	exit(EXIT_FAILURE);
}  /* end sock_err_sys_q */

/*
 * win_err_sys —— 输出win-sdk的错误信息（包括系统的错误信息）
 */
void win_err_sys(const char *cause, ...)
{
	va_list ap;

	va_start(ap, cause);
	err_handle(3, cause, ap);
	va_end(ap);
	return;
}  /* end win_err_sys */

/*
 * win_err_sys_q —— 输出win-sdk的错误信息（包括系统的错误信息），并退出
 */
void win_err_sys_q(const char *cause, ...)
{
	va_list ap;

	va_start(ap, cause);
	err_handle(3, cause, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}  /* end win_err_sys_q */

/*
 * err_handle —— 主要的错误处理函数
 */
void err_handle(int errnoflg, const char *fmt, va_list ap)
{
	int		errno_save, n;
	char	line[MAXLINE], *msgbuff = NULL;

	vsprintf(line, fmt, ap);
	if (1 == errnoflg) {
		n = strlen(line);
		errno_save = errno;
		sprintf(line+n, ": %s", strerror(errno));
		errno = errno_save;
	} else if (errnoflg > 1){
		strcat(line, ": ");
		n = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | 40, NULL, 
			2 == errnoflg ? WSAGetLastError() : GetLastError(), 
			MAKELANGID(0, SUBLANG_ENGLISH_US), (LPTSTR) &msgbuff, MAXLINE, NULL);
		strcat(line, msgbuff);
		if (n > 0)
			LocalFree((HLOCAL) msgbuff);
	}
	strcat(line, "\n\r");
	fflush(stdout);
	usr_cprintf(line, stderr);
	fflush(stderr);
	return;
} /* end of err_handle */
