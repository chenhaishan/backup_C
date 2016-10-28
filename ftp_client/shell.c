#include <io.h>
#include <fcntl.h>
#include <direct.h>
#include <sys/stat.h>
#include <time.h>
#include "myconio.h"
#include "dxyh.h"
#include "shell.h"
#include "error.h"

static bool	do_shell_flg	= TRUE;
static bool echo_on_flg		= TRUE;
static char cur_directory[MAX_PATH];
static Status shell_once(const char *arg);
static Status shell(void);
static Status shell_clear(const char *arg);
static Status shell_exit(const char *arg);
static Status shell_list(const char *arg);
static Status show_single_item(const char *filename);
static Status show_directory(const char *dir);
static Status shell_cd(const char *arg);
static Status shell_pwd(const char *arg);
static Status shell_rename(const char *arg);
static Status shell_mv(const char *arg);
static Status shell_cp(const char *arg);
static Status shell_mkdir(const char *arg);
static Status shell_rmdir(const char *arg);
static Status shell_delete(const char *arg);
static Status shell_type(const char *arg);
static Status shell_wc(const char *arg);
static Status shell_echo(const char *arg);
static Status shell_more(const char *arg);
static Status shell_edit(const char *arg);

const SHELL_CMD shell_cmd_table[] = {
	{ "clear",	shell_clear },
	{ "cls",	shell_clear },
	{ "exit",	shell_exit },
	{ "list",	shell_list },
	{ "ls",		shell_list },
	{ "dir",	shell_list },
	{ "cd",		shell_cd },
	{ "pwd",	shell_pwd },
	{ "rename", shell_rename },
	{ "move",	shell_mv },
	{ "mv",		shell_mv },
	{ "copy",	shell_cp },
	{ "cp",		shell_cp },
	{ "mkdir",	shell_mkdir },
	{ "rmdir",	shell_rmdir },
	{ "delete",	shell_delete },
	{ "cat",	shell_type },
	{ "type",	shell_type },
	{ "wc",		shell_wc },
	{ "echo.",	shell_echo },
	{ "more",	shell_more },
	{ "edit",	shell_edit },
	{ NULL, NULL }
};

static Status shell_clear(const char *arg)
{
	clrscr();
	return SHELL_OK;
}

static Status shell_exit(const char *arg)
{
	clrscr();
	do_shell_flg = FALSE;
	return SHELL_OK;
}

static Status shell_list(const char *arg)
{
	if (NULL == arg)
		show_directory("*.*");
	else {
		struct _stat st;

		if (-1 == _stat(arg, &st)) {
			usr_cprintf("Cannot find the file or directory\n");
			return SHELL_ERR;
		}
		if (st.st_mode & _S_IFDIR) {
			char	tmppath[MAX_PATH];
			strcpy(tmppath, arg);
			strcat(tmppath, "\\*.*");
			show_directory(tmppath);
		}
		else if (st.st_mode & _S_IFREG)
			show_single_item(arg);
	}
	return SHELL_OK;
}

static Status show_directory(const char *dir)
{
	WIN32_FIND_DATA	find_file_data;
	HANDLE			h_find;
	char			tmpname[MAX_PATH], *p = NULL;

	h_find = FindFirstFile(dir, &find_file_data);
	if (INVALID_HANDLE_VALUE == h_find) {
		win_err_sys("FindFirstFile error");
		return SHELL_ERR;
	}
		// ignore file "." and ".."
	// show_single_item(find_file_data.cFileName);
	while (FindNextFile(h_find, &find_file_data)) {
		if ((find_file_data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) ||
			(find_file_data.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) ||
			(0 == strcmp(find_file_data.cFileName, "..")))
			continue;
		strcpy(tmpname, dir);
		p = strstr(tmpname, "*.*");
		*p = '\0';
		strcat(tmpname, find_file_data.cFileName);
		show_single_item(tmpname);
	}
	FindClose(h_find);
	return SHELL_OK;
}

static Status show_single_item(const char *filename)
{
	char	mode[] = "----";
	char	buff[BUFSIZE], timebuf[MAXLINE];
	struct _stat st;
	int		timelen, off_set;
	struct tm *ptm = NULL;

	if (-1 == _stat(filename, &st)) {
		std_err_sys("_stat error");
		return SHELL_ERR;
	}

	if (st.st_mode & _S_IFDIR)
		mode[0] = 'd';
	else if (st.st_mode & _S_IFIFO)
		mode[0] = 'p';
	if (st.st_mode & _S_IREAD)
		mode[1] = 'r';
	if (st.st_mode & _S_IWRITE)
		mode[2] = 'w';
	if (st.st_mode & _S_IEXEC)
		mode[3] = 'x';
	off_set = 0;
	off_set += sprintf(buff + off_set, "%s", mode);
	off_set += sprintf(buff + off_set, "%*d", 8, (int) st.st_size);
	ptm = localtime(&st.st_mtime);
	if (ptm != NULL &&
		(timelen = strftime(timebuf, sizeof(timebuf), "    %b %d %H:%S", ptm)) > 0) {
		timebuf[timelen] = '\0';
		off_set += sprintf(buff + off_set, "%s", timebuf);
	} else {
		std_err_sys("localtime error");
		return SHELL_ERR;
	}
	off_set += sprintf(buff + off_set, "    %-12s\r\n", filename);
	usr_cprintf(buff);
	return SHELL_OK;
}

static Status shell_pwd(const char *arg)
{
	char	path[MAX_PATH];

	if (0 == GetCurrentDirectory(MAX_PATH, path)) {
		win_err_sys("GetCurrentDirectory error");
		return SHELL_ERR;
	}
	usr_cprintf("%s\r\n", path);
	return SHELL_OK;
}

static Status shell_cd(const char *arg)
{
	if (NULL == arg)
		return shell_pwd(NULL);
	if (!SetCurrentDirectory(arg)) {
		win_err_sys("SetCurrentDirectory error");
		return SHELL_ERR;
	}
	usr_cprintf("(current dir) ");
	textcolor(LIGHTGREEN);
	shell_pwd(NULL);
	textcolor(LIGHTGRAY);
	GetCurrentDirectory(MAX_PATH, cur_directory);
	return SHELL_OK;
}

static Status shell_rename(const char *arg)
{
	char	src_filepath[MAX_PATH], dst_filepath[MAX_PATH];
	const char *p = NULL, *q = NULL;
	int		i;

	if (NULL == arg) {
		usr_cprintf("The syntax of the command is incorrect.\r\n");
		return SHELL_ERR;
	}
	p = arg;
	i = 0;
	while (*p != '\0' && *p != ' ' && *p != '\t') {
		if ('\\' == *p) {
			usr_cprintf("The syntax of the command is incorrect.\r\n");
			return SHELL_ERR;
		}
		src_filepath[i++] = *p++;
	}
	src_filepath[i] = '\0';
	q = p;
	p = skip_blanks(q);
	if (NULL == p || '\0' == p)
		return SHELL_OK;
	i = 0;
	while (*p != '\0' && *p != ' ' && *p != '\t') {
		if ('\\' == *p) {
			usr_cprintf("The syntax of the command is incorrect.\r\n");
			return SHELL_ERR;
		}
		dst_filepath[i++] = *p++;
	}
	dst_filepath[i] = '\0';
	if (rename(src_filepath, dst_filepath) != 0) {
		usr_cprintf("A duplicate file name exists, or the filecannot be found.\r\n");
		return SHELL_ERR;
	}
	return SHELL_OK;
}

static Status shell_mv(const char *arg)
{
	char	src_filepath[MAX_PATH], dst_filepath[MAX_PATH],
			buff[BUFSIZE];
	const char	*p = NULL, *q = NULL;
	int		i, n, fd_src, fd_dst;
	struct _stat st;

	if (NULL == arg) {
		usr_cprintf("The syntax of the command is incorrect.\r\n");
		return SHELL_ERR;
	}
	i = 0;
	p = arg;
	while (*p != '\0' && *p != ' ' && *p != '\t')
		src_filepath[i++] = *p++;
	src_filepath[i] = '\0';
	q = p;
	p = skip_blanks(q);
	if (NULL == p || '\0' == p)
		return SHELL_OK;
	i = 0;
	while (*p != '\0' && *p != ' ' && *p != '\t')
		dst_filepath[i++] = *p++;
	dst_filepath[i] = '\0';

	if (0 == strcmp(src_filepath, dst_filepath))
		return SHELL_OK;

	if (-1 == (fd_src = _open(src_filepath, _O_RDONLY, 0))) {
		std_err_sys("_open error");
		return SHELL_ERR;
	}

	if (_stat(dst_filepath, &st) != 0) {
		if (-1 == (fd_dst = _open(dst_filepath, _O_CREAT | _O_WRONLY, _S_IREAD | _S_IWRITE))) {
			usr_cprintf("_open dest-file error.\r\n");
			close(fd_src);
			return SHELL_ERR;
		}
	} else {
		if (st.st_mode & _S_IFDIR) {
			WIN32_FIND_DATA	find_file_data;
			HANDLE			h_find;
			char			tmpfilepath[MAX_PATH], str[MAX_CHAR_LINE];
			bool			same_as_subdirflg	= FALSE,
							same_as_subfileflg	= FALSE;
			char			*s = NULL;

			strcpy(tmpfilepath, dst_filepath);
			strcat(tmpfilepath, "\\*.*");
			h_find = FindFirstFile(tmpfilepath, &find_file_data);
			if (INVALID_HANDLE_VALUE == h_find) {
				close(fd_src);
				return SHELL_ERR;
			}
			s = strrchr(src_filepath, '\\');
			while (FindNextFile(h_find, &find_file_data)) {
				if (((s != NULL) && (0 == strcmp(s+1, find_file_data.cFileName))) ||
					(0 == strcmp(src_filepath, find_file_data.cFileName))) {
					if (find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
						same_as_subdirflg = TRUE;
						break;
					} else {
						same_as_subfileflg = TRUE;
						break;
					}
				}
			}
			FindClose(h_find);

			if (same_as_subdirflg) {
				usr_cprintf("Access denied.\r\n");
				usr_cprintf("        0 file(s) moved.\r\n");
				close(fd_src);
				return SHELL_ERR;
			}

			if (same_as_subfileflg) {
				usr_cprintf("Overwrite %s? (Yes/No/All): ",
								NULL == s ? src_filepath : s+1);
				p = usr_cgetline(str, MAX_CHAR_LINE);
				q = skip_blanks(p);
				if (*q != 'y' && *q != 'Y') {
					close(fd_src);
					return SHELL_OK;
				}
			}
			sprintf(tmpfilepath, "%s\\%s", dst_filepath,
						NULL == s ? src_filepath : s+1);
			if (-1 == (fd_dst = _open(tmpfilepath, _O_CREAT | _O_WRONLY, _S_IREAD | _S_IWRITE))) {
				usr_cprintf("_open dest-file error.\r\n");
				close(fd_src);
				return SHELL_ERR;
			}
		}
	}
	while ((n = _Read(fd_src, buff, sizeof(buff))) != 0)
		Writen(fd_dst, buff, n);
	close(fd_src);
	close(fd_dst);
	if (_unlink(src_filepath) != 0) {
		std_err_sys("delete source file error");
		return SHELL_ERR;
	}
	return SHELL_OK;
}

static Status shell_cp(const char *arg)
{
	char	src_filepath[MAX_PATH], dst_filepath[MAX_PATH],
			buff[BUFSIZE], str[MAX_CHAR_LINE];
	const char *p = NULL, *q = NULL;
	int		i, n, fd_src, fd_dst;
	struct _stat st;

	if (NULL == arg) {
		usr_cprintf("The syntax of the command is incorrect.\r\n");
		return SHELL_ERR;
	}
	i = 0;
	p = arg;
	while (*p != '\0' && *p != ' ' && *p != '\t')
		src_filepath[i++] = *p++;
	src_filepath[i] = '\0';
	q = p;
	p = skip_blanks(q);
	if (NULL == p || '\0' == p) {
		usr_cprintf("The file cannot be copied onto itself.\r\n");
		usr_cprintf("        0 file(s) copied.\r\n");
		return SHELL_ERR;
	}
	i = 0;
	while (*p != '\0' && *p != ' ' && *p != '\t')
		dst_filepath[i++] = *p++;
	dst_filepath[i] = '\0';
		// must have two args, neither more or less is ok.
	q = p;
	p = skip_blanks(q);
	if (p != NULL && *p != '\0') {
		usr_cprintf("The syntax of the command is incorrect.\r\n");
		return SHELL_ERR;
	}

	if (0 == strcmp(src_filepath, dst_filepath)) {
		usr_cprintf("The file cannot be copied onto itself.\r\n");
		usr_cprintf("        0 file(s) copied.\r\n");
		return SHELL_ERR;
	}

	if (-1 == (fd_src = _open(src_filepath, _O_RDONLY, 0))) {
		usr_cprintf("The system cannot find the file specified.\r\n");
		return SHELL_ERR;
	}

	if (_stat(dst_filepath, &st) != 0) {
		if (-1 == (fd_dst = _open(dst_filepath, _O_CREAT | _O_WRONLY, _S_IREAD | _S_IWRITE))) {
			usr_cprintf("_open dest-file error.\r\n");
			close(fd_src);
			return SHELL_ERR;
		}
	} else {
		if (st.st_mode & _S_IFREG) {
			usr_cprintf("Overwrite %s? (Yes/No/All): ", dst_filepath);
			p = usr_cgetline(str, MAX_CHAR_LINE);
			q = skip_blanks(p);
			if (*q != 'y' && *q != 'Y') {
				close(fd_src);
				return SHELL_OK;
			}
			if (-1 == (fd_dst = _open(dst_filepath, _O_CREAT | _O_WRONLY, _S_IREAD | _S_IWRITE))) {
				usr_cprintf("_open dest-file error.\r\n");
				close(fd_src);
				return SHELL_ERR;
			}
		} else if (st.st_mode & _S_IFDIR) {
			WIN32_FIND_DATA	find_file_data;
			HANDLE			h_find;
			char			tmpfilepath[MAX_PATH];
			bool			same_as_subdirflg	= FALSE,
							same_as_subfileflg	= FALSE;
			char			*s = NULL;

			strcpy(tmpfilepath, dst_filepath);
			strcat(tmpfilepath, "\\*.*");
			h_find = FindFirstFile(tmpfilepath, &find_file_data);
			if (INVALID_HANDLE_VALUE == h_find) {
				close(fd_src);
				return SHELL_ERR;
			}
			s = strrchr(src_filepath, '\\');
			while (FindNextFile(h_find, &find_file_data)) {
				if (((s != NULL) && (0 == strcmp(s+1, find_file_data.cFileName))) ||
					(0 == strcmp(src_filepath, find_file_data.cFileName))) {
					if (find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
						same_as_subdirflg = TRUE;
						break;
					} else {
						same_as_subfileflg = TRUE;
						break;
					}
				}
			}
			FindClose(h_find);

			if (same_as_subdirflg) {
				usr_cprintf("Access denied.\r\n");
				usr_cprintf("        0 file(s) copied.\r\n");
				close(fd_src);
				return SHELL_ERR;
			}

			if (same_as_subfileflg) {
				usr_cprintf("Overwrite %s? (Yes/No/All): ",
								NULL == s ? src_filepath : s+1);
				p = usr_cgetline(str, MAX_CHAR_LINE);
				q = skip_blanks(p);
				if (*q != 'y' && *q != 'Y') {
					close(fd_src);
					return SHELL_OK;
				}
			}
			sprintf(tmpfilepath, "%s\\%s", dst_filepath,
						NULL == s ? src_filepath : s+1);
			if (-1 == (fd_dst = _open(tmpfilepath, _O_CREAT | _O_WRONLY, _S_IREAD | _S_IWRITE))) {
				usr_cprintf("_open dest-file error.\r\n");
				close(fd_src);
				return SHELL_ERR;
			}
		}
	}
	while ((n = _Read(fd_src, buff, sizeof(buff))) > 0)
		Writen(fd_dst, buff, n);
	close(fd_dst);
	close(fd_src);
	return SHELL_OK;
}

static Status shell_mkdir(const char *arg)
{
	char		filename[MAX_PATH];
	const char	*p = NULL, *q = NULL;
	int			i;
	Status		retval;

	if (NULL == arg) {
		usr_cprintf("The syntax of the command is incorrect.\r\n");
		return SHELL_ERR;
	}

	retval = SHELL_OK;
	p = arg;
	for ( ; ; ) {
		i = 0;
		while (*p != '\0' && *p != ' ' && *p != '\t')
			filename[i++] = *p++;
		filename[i] = '\0';
		q = p;
		p = skip_blanks(q);
		if (_mkdir(filename) != 0) {
			usr_cprintf("A subdirectory or file dst already exists.\r\n");
			if (*p != '\0')
				usr_cprintf("Error occurred while processing: %s.\r\n", filename);
			retval = SHELL_ERR;
		}
		if (NULL == p || '\0' == *p)
			break;
	}
	return retval;
}

static Status shell_rmdir(const char *arg)
{
	char		filename[MAX_PATH];
	const char	*p = NULL, *q = NULL;
	int			i;
	Status		retval;
	

	if (NULL == arg) {
		usr_cprintf("The syntax of the command is incorrect.\r\n");
		return SHELL_ERR;
	}

	retval = SHELL_OK;
	p = arg;
	for ( ; ; ) {
		i = 0;
		while (*p != '\0' && *p != ' ' && *p != '\t')
			filename[i++] = *p++;
		filename[i] = '\0';
		q = p;
		p = skip_blanks(q);
		if (_rmdir(filename) != 0)
			retval = SHELL_ERR;
		if (NULL == p || '\0' == *p)
			break;
	}
	if (SHELL_ERR == retval)
		usr_cprintf("The system cannot find the file specified, or the\r\n"
					"directory specified is not empty.\r\n");
	return retval;
}

static Status shell_delete(const char *arg)
{
	if (NULL == arg) {
		usr_cprintf("The syntax of the command is incorrect.\r\n");
		return SHELL_ERR;
	} else {
		char		filename[MAX_PATH];
		const char	*p = NULL, *q = NULL;
		struct _stat st;
		int			i;
		Status		retval;
		
		p = arg;
		retval = SHELL_OK;
		for ( ; ; ) {
			i = 0;
			while (*p != '\0' && *p != ' ' && *p != '\t')
				filename[i++] = *p++;
			filename[i] = '\0';
			if (_stat(filename, &st) != 0)
				retval = SHELL_ERR;
			else {
				char	str[MAX_CHAR_LINE], *s1 = NULL, *s2 = NULL;

				if (st.st_mode & _S_IFDIR) {
					usr_cprintf("%s\\*.*, Are you sure (Y/N)? ", filename);
					s1 = usr_cgetline(str, MAX_CHAR_LINE);
					s2 = skip_blanks(s1);
					if ('y' == *s2 || 'Y' == *s2) {
						WIN32_FIND_DATA	find_file_data;
						HANDLE			h_find;
						char			file2delete[MAX_PATH];

						strcat(filename, "\\*.*");
						h_find = FindFirstFile(filename, &find_file_data);
						if (INVALID_HANDLE_VALUE == h_find) {
							retval = SHELL_ERR;
							goto again;
						}
						while (FindNextFile(h_find, &find_file_data)) {
							if (find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
								continue;
							strcpy(file2delete, filename);
							s1 = strstr(file2delete, "*.*");
							strcpy(s1, find_file_data.cFileName);
							if (unlink(file2delete) != 0)
								retval = SHELL_ERR;
						}
						FindClose(h_find);
					}
				} else {
					if (_unlink(filename) != 0)
						retval = SHELL_ERR;
				}
			}
again:
			q = p;
			p = skip_blanks(q);
			if (NULL == p || '\0' == *p)
				break;
		}
		if (SHELL_ERR == retval)
			usr_cprintf("The system cannot find the file specified.\r\n");
		return SHELL_OK;
	}
}

static Status shell_type(const char *arg)
{
	if (NULL == arg) {
		usr_cprintf("The syntax of the command is incorrect.\r\n");
		return SHELL_ERR;
	} else {
		const char	*p = NULL, *q = NULL;
		char		filename[MAX_PATH];
		int			c, filenum, i;
		FILE		*fp = NULL;

		p = arg;
		filenum = 0;
		for ( ; ; ) {
			i = 0;
			while (*p != '\0' && *p != ' ' && *p != '\t')
				filename[i++] = *p++;
			filename[i] = '\0';
			usr_cprintf("%dth file: ", ++filenum);
			usr_cprintfcolor(LIGHTGREEN, "%s\r\n", filename);
			if (NULL == (fp = fopen(filename, "r"))) {
				std_err_sys("fopen file error");
				return SHELL_ERR;
			}
			while ((c = fgetc(fp)) != EOF) {
				if ('\n' == c)
					usr_putchar('\r');
				usr_putchar(c);
			}
			fclose(fp);
			usr_cprintf("\r\n");
			q = p;
			p = skip_blanks(q);
			if (NULL == p || '\0' == *p)
				break;
		}
		return SHELL_OK;
	}
}

static Status shell_wc(const char *arg)
{
	enum {
		WC_LINES	= 0,
		WC_WORDS	= 1,
		WC_CHARS	= 2,
	};
	const char *p = NULL, *q = NULL;
	char	filename[MAX_PATH];
	size_t	linepos, counts[3], totals[3];
	int		i, c, in_word, num_files = 0;
	FILE	*fp = NULL;
	Status	exit_status = SHELL_OK;

	memset(totals, 0, sizeof(totals));
	p = arg;
	do {
		++num_files;
			// get the filename
		i = 0;
		while (*p != ' ' && *p != '\t' && *p != '\0')
			filename[i++] = *p++;
		filename[i] = '\0';
			// open file to "wc"
		if (NULL == (fp = fopen(filename, "r"))) {
			std_err_sys("wc: %s", filename);
			exit_status = SHELL_ERR;
			continue;
		}
			/*
			 * counts[0]	--> count lines
			 * counts[1]	--> count worlds
			 * counts[2]	--> count chars
			 */
		memset(counts, 0, sizeof(counts));
		linepos = 0;
		in_word = 0;
		do {
			++counts[WC_CHARS];
			c = getc(fp);
			if (Isprint(c)) {
				++linepos;
				if (!Isspace(c)) {
					in_word = 1;
					continue;
				}
			} else if (((unsigned int) (c - 9)) <= 4) {
				if ('\t' == c)
					linepos = (linepos | 7) + 1;
				else {
				DO_EOF:
					if ('\n' == c)
						++counts[WC_LINES];
					linepos = 0;
				}
			} else if (EOF == c) {
				if (ferror(fp)) {
					std_err_sys("wc: %s", filename);
					exit_status = SHELL_ERR;
				}
				--counts[WC_CHARS];
				goto DO_EOF;
			} else
				continue;
			counts[WC_WORDS] += in_word;
			in_word = 0;
			if (EOF == c)
				break;
		} while (1);
		fclose(fp);
		usr_cprintf("%5d  %5d  %6d  %-8s\r\n",
			++counts[WC_LINES], counts[WC_WORDS], counts[WC_CHARS], filename);
		for (i = 0; i < 3; ++i)
			totals[i] += counts[i];
		q = p;
		p = skip_blanks(q);
		if (NULL == p || '\0' == *p)
			break;
	} while (1);
	if (num_files > 1)
		usr_cprintf("%5d  %5d  %6d  %-8s\r\n",
			totals[WC_LINES], totals[WC_WORDS], totals[WC_CHARS], "total");
	return SHELL_OK;
}

static Status shell_echo(const char *arg)
{
	if (NULL == arg) {
		usr_cprintf("ECHO is %s.\r\n",
			echo_on_flg ? "on" : "off");
		return SHELL_OK;
	} else {
		if (0 == stricmp(arg, "ON")) {
			echo_on_flg = TRUE;
			return SHELL_OK;
		} else if (0 == stricmp(arg, "OFF")) {
			echo_on_flg = FALSE;
			return SHELL_OK;
		} else if (0 == strnicmp(arg, ">+>ECHO.<+<", 11)) {
			usr_cprintf("\r\n");
			return SHELL_OK;
		} else {
			if (NULL ==strchr(arg, '>'))
				usr_cprintf("%s\r\n", arg);
			else {
				int			single_grater_mark_counter, double_grater_mark_counter,
							num_args;
				const char	*p1 = NULL, *q1 = NULL,
							*p2 = NULL, *q2 = NULL;
				
					// first check whether has the syntax error
				if ('>' == *arg) {
					usr_cprintf("The syntax of the command is incorrect.\r\n");
					return SHELL_ERR;
				}
				single_grater_mark_counter	= 
				double_grater_mark_counter	= 
				num_args					= 0;
				q2 = arg;
				for ( ; ; ) {
					p2 = strstr(q2, ">>");
					if (NULL == p2)
						break;
					++double_grater_mark_counter;
					q2 = p2 + 2;
				}
				q1 = arg;
				for ( ; ; ) {
					p1 = strchr(q1, '>');
					if (NULL == p1)
						break;
					if ('>' == *(p1+1)) {
						if ('>' == *(p1+1+1)) {
							usr_cprintf("> was unexpected at this time.\r\n");
							return SHELL_ERR;
						}
						q1 = p1 + 2;
						continue;
					}
					++single_grater_mark_counter;
					q1 = p1 + 1;
				}
				num_args = 1;
				q1 = arg;
				for ( ; ; ) {
					p1 = strchr(q1, '>');
					if (NULL == p1)
						break;
					if ('>' == *(p1+1))
						q1 = p1 + 2;
					else
						q1 = p1 + 1;
					++num_args;
				}
					// num_args must be equal to single_grater_mark_counter + double_grater_mark_counter + 1
				if (num_args != (single_grater_mark_counter + double_grater_mark_counter + 1)) {
					usr_cprintf("The syntax of the command is incorrect.\r\n");
					return SHELL_ERR;
				} else {
					FILE	*fp_src = NULL, *fp_dst = NULL;
					char	src_filepath[MAX_PATH], dst_filepath[MAX_PATH],
							line[MAXLINE], buff[BUFSIZE];
					const char *p = NULL, *q = NULL;
					int		i, n;
					bool	appendflg;

					p = arg;
					i = 0;
					while (*p != '>')
						line[i++] = *p++;
					line[i] = '\0';
					appendflg = FALSE;
					if (*p != '>')
						p = strchr(arg, '>');
					if ('>' == *(p+1)) {
						q = p + 2;
						appendflg = TRUE;
					} else
						q = p + 1;
					p = skip_blanks(q);
					i = 0;
					while (*p != '\0' && *p != ' ' && *p != '>')
						dst_filepath[i++] = *p++;
					dst_filepath[i] = '\0';
					if (NULL == (fp_dst = fopen(dst_filepath,
						appendflg ? "a" : "w"))) {
						std_err_sys("%s to file error",
							appendflg ? "append" : "write");
						return SHELL_ERR;
					}
					fprintf(fp_dst, line);
					fclose(fp_dst);
					for ( ; ; ) {
						strcpy(src_filepath, dst_filepath);
						appendflg = FALSE;
						if (*p != '>') {
							q = p;
							p = strchr(q, '>');
							if (NULL == p)
								break;
						}
						if ('>' == *p && '>' == *(p+1)) {
							appendflg = TRUE;
							q = p + 2;
						} else
							q = p + 1;
						p = skip_blanks(q);
						i = 0;
						while (*p != '\0' && *p != ' ' && *p != '>')
							dst_filepath[i++] = *p++;
						dst_filepath[i] = '\0';
						if (NULL == (fp_src = fopen(src_filepath, "rb"))) {
							std_err_sys("%s to file %s error",
								appendflg ? "append" : "write",
								dst_filepath);
							return SHELL_ERR;
						}
						if (NULL == (fp_dst = fopen(dst_filepath,
							appendflg ? "ab" : "wb"))) {
							std_err_sys("%s to file %s error",
								appendflg ? "append" : "write",
								dst_filepath);
							fclose(fp_src);
							return SHELL_ERR;
						}
						while (!feof(fp_src)) {
							n = fread(buff, sizeof(char), sizeof(buff), fp_src);
							fwrite(buff, sizeof(char), n, fp_dst);
						}
						if (ferror(fp_src) || ferror(fp_dst)) {
							std_err_sys("Sth is wrong while %s to file %s",
								appendflg ? "appending" : "writing",
								dst_filepath);
							fclose(fp_src);
							fclose(fp_dst);
							return SHELL_ERR;
						}
						fclose(fp_src);
						fclose(fp_dst);
						if ('\0' == *p)
							break;
					}
				}
			}
		}
		return SHELL_OK;
	}
}

static Status shell_more(const char *arg)
{
	if (NULL == arg) {
		usr_cprintf("usage: more <[file1] [file2] [file3] ...>\r\n");
		return SHELL_ERR;
	} else {
		const char	*p = NULL, *q = NULL, *save_p = NULL;
		char		filename[MAX_PATH];
		int			i, show_area_height, show_area_width,
					page_height, page_width,
					c, lines, len;
		FILE		*fp = NULL;
		struct _stat st;
		bool		have_doneflg = FALSE;

		show_area_width		= get_curwin_width();
		show_area_height	= get_curwin_height();
		page_height	= show_area_height - 2;
		if (!echo_on_flg)
			page_height += 1;
		page_width	= show_area_width - 1;
		p = skip_blanks(arg);
		for ( ; ; ) {
			i = 0;
			while (*p != ' ' && *p != '\0' && *p != '\t')
				filename[i++] = *p++;
			filename[i] = '\0';

			if (have_doneflg) {
				usr_cprintfcolor(WHITE+(RED<<4), "--More--(Next file: %s)", filename);
				c = usr_getch();
				if ('q' == c)
					goto again;
				usr_putchar('\r');
				delline();
			}

			if (_stat(filename, &st) != 0) {
				std_err_sys("\r\nmore file \"%s\" failed", filename);
				goto again;
			}

			if (NULL == (fp = fopen(filename, "r"))) {
				std_err_sys("\r\nopen file \"%s\" to more error", filename);
				goto again;
			}

			if (echo_on_flg) {
				usr_cprintf("more file: ");
				usr_cprintfcolor(LIGHTGREEN, "%s\r\n", filename);
			}
			len = 0;
			lines = 0;
			while ((c = getc(fp)) != EOF) {
				if ('\n' == c) {
					usr_putchar('\r');
					usr_putchar('\n');
					len = 0;
					++lines;
				} else {
					usr_putchar(c);
					if (++len >= page_width) {
						usr_cprintf("\r\n");
						len = 0;
						++lines;
					}
				}
				if (lines >= page_height) {
					usr_cprintfcolor(WHITE+(RED<<4), "--More--(%2d%%)",
						(int) (100 *((double) ftell(fp)/(double) st.st_size)));
					for ( ; ; ) {
						c = usr_getch();
						if (' ' == c || KEY_ENTER == c || KEY_ESC == c)
							break;
					}
					if (KEY_ESC == c) {
						clrscr();
						return SHELL_OK;
					}
					usr_putchar('\r');
					delline();
					if (KEY_ENTER == c) {
						usr_cprintf("\r\n");
						gotoxy(1, wherey()-1);
						--lines;
					}
					else if (' ' == c)
						lines = 0;
					len = 0;
				}
			}
			fclose(fp);
again:
			usr_cprintf("\r\n");
			q = p;
			p = skip_blanks(q);
			if (NULL == p || '\0' == *p)
				break;
			have_doneflg = TRUE;
		}
		return SHELL_OK;
	}
}

static Status shell_edit(const char *arg)
{
	int		win_left, win_top, win_right, win_bottom;
	int		cur_x, cur_y;
	char	save_scr_buff[80*25*4];

		// save the environment
	get_curwin(&win_left, &win_top, &win_right, &win_bottom);
	wherexy(&cur_x, &cur_y);
	gettext(1, 1, 80, 25, save_scr_buff);
	if (NULL == arg)
		system("edit");
	else {
		char	filename[MAX_PATH], cmdline[MAX_PATH];
		const char *p = NULL, *q = NULL;
		int		i;

		p = arg;
		for ( ; ; ) {
			i = 0;
			while (*p != ' ' && *p != '\t' && *p != '\0')
				filename[i++] = *p++;
			filename[i] = '\0';
				// not implemented yet, so use dos cmd instead
			sprintf(cmdline, "edit %s", filename);
			usr_cprintf("cmdline is: %s\r\n", cmdline);
			system(cmdline);
			q = p;
			p = skip_blanks(q);
			if (NULL == p || '\0' == *p)
				break;
		}
	}
	// restore the environment
	puttext(1, 1, 80, 25, save_scr_buff);
	window(win_left, win_top, win_right, win_bottom);
	gotoxy(cur_x, cur_y);
	return SHELL_OK;
}

static Status shell(void)
{
	char	str[MAX_CHAR_LINE], *p = NULL, *q = NULL;
	const char *slash_ptr = NULL;

	for ( ; ; ) {
		if (echo_on_flg)
			usr_cprintf("[%s]# ", cur_directory);
		p = usr_cgetline(str, MAX_CHAR_LINE);
		q = p;
		p = skip_blanks(q);
		if ('\0' == *p)
			continue;
		shell_once(p);
		if (!do_shell_flg)
			break;
	}
	return SHELL_OK;
}

static Status shell_once(const char *arg)
{
	char	cmd[CMD_LEN];
	const char *p = NULL, *q = NULL;
	int		i, k, possible_cmds;

	p = arg;
	i = 0;
	while (*p != ' ' && *p != '\t' && *p != '\0' && *p != ';')
		cmd[i++] = *p++;
	cmd[i] = '\0';
	possible_cmds = 0;
	for (i = 0; shell_cmd_table[i].cmd != NULL; ++i) {
		if (strlen(cmd) > 0 &&
			(0 == strncmp(cmd, shell_cmd_table[i].cmd, strlen(cmd)))) {
			++possible_cmds;
			k = i;
		}
	}
	if (possible_cmds != 1) {
		usr_cprintf("+shell: %s: command not found\r\n", cmd);
		return SHELL_ERR;
	} else {
		q = p;
		p = skip_blanks(q);
		if ('\0' == *p) {
			if (0 == stricmp(cmd, "ECHO."))
				shell_cmd_table[k].cmd_handler(">+>ECHO.<+<");
			else
				shell_cmd_table[k].cmd_handler(NULL);
		}
		else
			shell_cmd_table[k].cmd_handler(p);
	}
	return SHELL_OK;
}

Status do_shell(const char *shell_cmd)
{
	const char	*p = NULL;

	do_shell_flg = TRUE;
	echo_on_flg = TRUE;
	GetCurrentDirectory(MAX_PATH, cur_directory);
	p = skip_blanks(shell_cmd);
	if (NULL == p || '\0' == *p) {
		while (do_shell_flg)
			shell();
	} else
		shell_once(p);

	return SHELL_OK;
}
