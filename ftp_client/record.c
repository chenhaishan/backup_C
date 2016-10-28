#include <time.h>
#include "myconio.h"
#include "dxyh.h"
#include "record.h"
#include "error.h"
#include "ftp.h"

static void lprintf(log_t *log, unsigned int level, bool err_flg,
					const char *fmt, va_list ap);

log_t	*logfp = NULL;

/*
 * log_open —— 打开日志
 */
log_t *log_open(const char *filename, int flags)
{
	if (!ftp_recordflg)	// 若已关闭日志模式则直接返回
		return NULL;
	else {
		log_t *log = NULL;

		if (NULL == (log = (log_t *) malloc(sizeof(log_t)))) {
			FTP_DEBUG_MSG("log_open error: unable to malloc");
			goto log_open_exit_a;
		}
		log->flags = flags;
		log->fp = fopen(filename, ((flags & LOG_O_TRUNC) ? "w" : "a"));
		if (NULL == log->fp) {
			FTP_DEBUG_MSG("log_open error: open logfile error");
			goto log_open_exit_b;
		}
		return log;
log_open_exit_b:
		free(log);
log_open_exit_a:
		ftp_recordflg = FALSE;
		return NULL;
	}
}

/*
 * log_close —— 关闭日志
 */
void log_close(log_t *logfp)
{
	if (ftp_recordflg) {
		fclose(logfp->fp);
		free(logfp);
	}
}

/*
 * log_msg —— 往日志文件中写入内容
 */
void log_msg(log_t *log, unsigned int level, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	lprintf(log, level, FALSE, fmt, ap);
	va_end(ap);
	return;
}

/*
 * log_sys —— 往日志文件中写入内容，错误发生时加上系统的错误信息
 */
void log_sys(log_t *log, unsigned int level, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	lprintf(log, level, TRUE, fmt, ap);
	va_end(ap);
	return;
}

/*
 * lprintf —— 往日志写入内容的主要函数
 * 
 * 例如：Wed Dec 08 13:50:44 [warn] >> Hello, this comes from the logfile.
 */
static void lprintf(log_t *log, unsigned int level, bool err_flg,
					const char *fmt, va_list ap)
{
	FILE	*fp = NULL;
	time_t	now;
	char	date[50], line[MAXLINE];
	int		n, errno_save;
	static char *levels[10] = {
		"[(bad)]", "[debug]", "[info]", "[warn]",
		"[error]", "[emerg]", "[fatal]"};

	if (NULL == log) return;
	fp = log->fp;
	if (!(log->flags&LOG_O_NODATE)) {
		now = time(NULL);
		strcpy(date, ctime(&now));
		date[strlen(date) - 6] = ' ';
		date[strlen(date) - 5] = '\0';
	}
	n = sprintf(line, "%s%s >> ",
			log->flags&LOG_O_NODATE ? "" : date,
			log->flags&LOG_O_NOLVL ? "" : 
			(level > LVL_FATAL ? levels[0] : levels[level]));
	vsprintf(line+n, fmt, ap);
	n = strlen(line);
	if (err_flg) {
		errno_save = errno;
		sprintf(line+n, ": %s", strerror(errno));
		errno = errno_save;
	}
	if (!(log->flags & LOG_O_NOLF))
		strcat(line, "\n");
	fprintf(fp, line);
	if (LVL_EMERG == level && (log->flags & LOG_O_STDERR)) {
		int		i, attr;
		attr = gettextattr();
		textattr(WHITE + (RED << 4));
		for (i = 0; line[i] != '\0'; ++i) {
			if ('\n' == line[i])
				usr_putchar('\r');
			usr_putchar(line[i]);
		}
		textattr(attr);
	}
}
