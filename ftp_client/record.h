#ifndef _RECORD_H
#define _RECORD_H

#define RECORD_OK	(0)
#define RECORD_ERR	(-1)

#define LVL_DEBUG		1
#define LVL_INFO		2
#define LVL_WARN		3
#define LVL_ERROR		4
#define LVL_EMERG		5
#define LVL_FATAL		6

#define LOG_O_TRUNC		(1 << 0)
#define LOG_O_NODATE	(1 << 1)
#define LOG_O_NOLF		(1 << 2)
#define LOG_O_NOLVL		(1 << 3)
#define LOG_O_DEBUG		(1 << 4)
#define LOG_O_STDERR	(1 << 5)
#define LOG_O_DEFAULT	(LOG_O_STDERR | LOG_O_TRUNC)

typedef struct _log_t {
	FILE	*fp;
	int		flags;
} log_t;

extern log_t *logfp;

extern log_t *log_open(const char *filename, int flags);
extern void log_close(log_t *logfp);
extern void log_msg(log_t *log, unsigned int level, const char *fmt, ...);
extern void log_sys(log_t *log, unsigned int level, const char *fmt, ...);

#endif
