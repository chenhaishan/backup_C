#ifndef _SHELL_H
#define _SHELL_H

#include "ftp.h"

#define SHELL_ERR	(-1)
#define SHELL_OK	(0)

typedef struct _SHELL_CMD {
	char	*cmd;
	Status	(*cmd_handler)(const char *arg);
} SHELL_CMD;

Status do_shell(const char *shell_cmd);

#endif
