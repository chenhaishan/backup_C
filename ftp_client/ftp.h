#ifndef _FTP_H
#define _FTP_H

#include <stdlib.h>		/* for _MAX_PATH */

#define FTP_ERR			(-1)
#define FTP_OK			(0)
#define ARR_LEN(arr)	(sizeof(arr) / sizeof(arr[0])
#define CMD_LEN			12
#define FTP_LINE_END	"\r\n"
#define USER_LEN		20
#define PASSWD_LEN		20

enum FILE_TRANS_TYPE { TYPE_A, TYPE_I };

extern bool	ftp_recordflg;
extern bool	ftp_debugflg;
extern char	logfile[_MAX_PATH];
extern char	ip_addr[16];
extern unsigned short port;
extern bool	ftp_runningflg;

typedef char	Status;
typedef struct _FTP_CMD {
	char	*cmd;
	Status	(*cmd_handler)(SOCKET sock_clit, const char *args);
	bool	may_have_argsflg;
} FTP_CMD;

#define FTP_DEBUG_MSG(msg)	FTP_DEBUG(__FILE__, __LINE__, msg)
void FTP_DEBUG(const char *filename, int lineno, const char *fmt, ...);
SOCKET ftp_connect2serv(void);
Status ftp_do_loop(SOCKET sock_clit);

#endif
