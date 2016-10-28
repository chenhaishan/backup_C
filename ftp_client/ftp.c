#include <fcntl.h>
#include <Winsock2.h>
#include "myconio.h"
#include "shell.h"
#include "dxyh.h"
#include "ftp.h"
#include "record.h"
#include "error.h"

bool	ftp_recordflg;
bool	ftp_debugflg = TRUE;
bool	ftp_runningflg = TRUE;
char	logfile[MAX_PATH];
char	ip_addr[16];
enum FILE_TRANS_TYPE ftp_type = TYPE_A;
unsigned short port;
static SOCKET sock_pasv;

static Status ftp_do_user(SOCKET sock_clit, const char *args);
static Status ftp_do_pass(SOCKET sock_clit, const char *args);
static Status ftp_do_syst(SOCKET sock_clit, const char *args);
static Status ftp_do_pwd(SOCKET sock_clit, const char *args);
static Status ftp_do_cd(SOCKET sock_clit, const char *args);
static Status ftp_do_quit(SOCKET sock_clit, const char *args);
static Status ftp_do_mkdir(SOCKET sock_clit, const char *args);
static Status ftp_do_rmdir(SOCKET sock_clit, const char *args);
static Status ftp_do_dele(SOCKET sock_clit, const char *args);
static Status ftp_do_size(SOCKET sock_clit, const char *args);
static Status ftp_do_pasv(SOCKET sock_clit, const char *args);
static Status ftp_do_list(SOCKET sock_clit, const char *args);
static Status ftp_do_nlst(SOCKET sock_clit, const char *args);
static Status ftp_do_type(SOCKET sock_clit, const char *args);
static Status ftp_do_ascii(SOCKET sock_clit, const char *args);
static Status ftp_do_bin(SOCKET sock_clit, const char *args);
static Status ftp_do_put(SOCKET sock_clit, const char *args);
static Status ftp_do_mput(SOCKET sock_clit, const char *args);
static Status ftp_do_get(SOCKET sock_clit, const char *args);
static Status ftp_do_mget(SOCKET sock_clit, const char *args);
static Status ftp_do_mdele(SOCKET sock_clit, const char *args);
static Status ftp_do_help(SOCKET sock_clit, const char *args);

const FTP_CMD ftp_cmds[] = {
	{"system",	ftp_do_syst,	FALSE},
	{"pwd",		ftp_do_pwd,		FALSE},
	{"cd",		ftp_do_cd,		TRUE},
	{"quit",	ftp_do_quit,	FALSE},
	{"bye",		ftp_do_quit,	FALSE},
	{"mkdir",	ftp_do_mkdir,	TRUE},
	{"rmdir",	ftp_do_rmdir,	TRUE},
	{"delete",	ftp_do_dele,	TRUE},
	{"mdelete",	ftp_do_mdele,	TRUE},
	{"size",	ftp_do_size,	TRUE},
	{"passive",	ftp_do_pasv,	FALSE},
	{"ls",		ftp_do_list,	TRUE},
	{"list",	ftp_do_list,	TRUE},
	{"nlist",	ftp_do_nlst,	TRUE},
	{"type",	ftp_do_type,	TRUE},
	{"binary",	ftp_do_bin,		FALSE},
	{"ascii",	ftp_do_ascii,	FALSE},
	{"put",		ftp_do_put,		TRUE},
	{"mput",	ftp_do_mput,	TRUE},
	{"get",		ftp_do_get,		TRUE},
	{"mget",	ftp_do_mget,	TRUE},
	{"help",	ftp_do_help,	FALSE},
	{"?",		ftp_do_help,	FALSE},
	{NULL, NULL, TRUE}
};

static void init_ftp_running_backg(void);

void FTP_DEBUG(const char *filename, int lineno, const char *fmt, ...)
{
	if (ftp_debugflg) {
		va_list		ap;
		char		line[MAXLINE];
		const char *p = NULL;
		int			n;

		p = strrchr(filename, '\\');
		n = sprintf(line, "[File:%s, Line:%d]: ",
			NULL == p ? filename : p+1,
			lineno);
		va_start(ap, fmt);
		vsprintf(line + n, fmt, ap);
		va_end(ap);
		usr_cprintf(line);
		return;
	}
}

SOCKET ftp_connect2serv(void)
{
	WORD		version_requested;
	WSADATA		wsadata;
	int			err;
	SOCKET		sock_clit;
	SOCKADDR_IN	serv_addr_in;

	init_ftp_running_backg();

	version_requested = MAKEWORD(1, 1);
	err = WSAStartup(version_requested, &wsadata);
	if (err != 0)
		win_err_sys_q("WSAStartup error");
	if (LOBYTE(wsadata.wVersion) != 1 ||
		HIBYTE(wsadata.wVersion) != 1)
		sock_err_sys_q("Cannot acquire the socket version specified");

	sock_clit = Socket(AF_INET, SOCK_STREAM, 0);
	memset(&serv_addr_in, 0, sizeof(SOCKADDR_IN));
	serv_addr_in.sin_family		= AF_INET;
	serv_addr_in.sin_port		= htons(port);
	serv_addr_in.sin_addr.S_un.S_addr = inet_addr(ip_addr);
	Connect(sock_clit, (SA *) &serv_addr_in, sizeof(SOCKADDR));
	usr_cprintf("Connected to %s\n\r", ip_addr);
	return sock_clit;
}

static void init_ftp_running_backg(void)
{
	window(1, 1, 80, 25);
	textattr(YELLOW+(BLUE<<4));
	clrscr();
	usr_cprintfxy(2, 2, "Simple ftp client tool...");
	box(3, 3, 78, 24, YELLOW+(BLUE<<4), TRUE);
	textattr(LIGHTGRAY);
	window(4, 4, 77, 23);
	clrscr();
}

int getreply(SOCKET sock_clit, char line[], int maxlen)
{
	int		ftp_code;
	char	*p = NULL;
	bool	calc_codeflg;

	if (NULL == line)
		return -1;
	Recv(sock_clit, line, maxlen, 0);
	ftp_code = 0;
	p = line;
	calc_codeflg = TRUE;
	while (*p != '\r') {
		if ((*p >= '0' && *p <= '9') && calc_codeflg) {
			ftp_code = ftp_code*10 + (*p - '0');
			if (*(p+1) < '0' || *(p+1) > '9')
				calc_codeflg = FALSE;
		}
		++p;
	}
		// wrong ftp line end
	if (*(p+1) != '\n') {}
	*p = '\0';
	return ftp_code;
}

static void ftp_send_info(SOCKET sock_clit, const char *fmt, ...)
{
	char		line[MAXLINE];
	va_list		ap;

	va_start(ap, fmt);
	vsprintf(line, fmt, ap);
	va_end(ap);
	strcat(line, FTP_LINE_END);
	Sendn(sock_clit, line, strlen(line), 0);
}

static Status ftp_do_user(SOCKET sock_clit, const char *args)
{
	char	str[MAX_CHAR_LINE+3], line[MAXLINE], *p = NULL;
	int		ftp_code;

	usr_cprintf("Name (%s:winsock): ", ip_addr);
	p = usr_cgetline(str, USER_LEN);
	ftp_send_info(sock_clit, "USER %s", p);
	ftp_code = getreply(sock_clit, line, sizeof(line));
	usr_cprintf("%s\r\n", line);
	if (550 == ftp_code) {
		usr_cprintf("Login failed.\r\n");
		return FTP_ERR;
	}
	return FTP_OK;
}

static Status ftp_do_pass(SOCKET sock_clit, const char *args)
{
	char	line[MAXLINE], str[MAX_CHAR_LINE+3], *p = NULL;
	int		ftp_code;

	usr_cprintf("Password: ");
	str[0] = MAX_CHAR_LINE;
	input_noecho(str);
	ftp_send_info(sock_clit, "PASS %s", &str[2]);
	ftp_code = getreply(sock_clit, line, sizeof(line));
	if (530 == ftp_code) {
		usr_cprintf("Login failed.\r\n");
		return FTP_ERR;
	}
	usr_cprintf("%s\r\n", line);
	return FTP_OK;
}

static Status ftp_do_syst(SOCKET sock_clit, const char *args)
{
	char	line[MAXLINE];
	int		ftp_code;

	ftp_send_info(sock_clit, "SYST");
	ftp_code = getreply(sock_clit, line, sizeof(line));
	usr_cprintf("%s\r\n", line);
	if (ftp_code != 215)
		return FTP_ERR;
	return FTP_OK;
}

static Status ftp_do_pwd(SOCKET sock_clit, const char *args)
{
	char	line[MAXLINE];
	int		ftp_code;

	ftp_send_info(sock_clit, "PWD");
	ftp_code = getreply(sock_clit, line, sizeof(line));
	usr_cprintf("%s\r\n", line);
	if (ftp_code != 257)
		return FTP_ERR;
	return FTP_OK;
}

static Status ftp_do_quit(SOCKET sock_clit, const char *args)
{
	char	line[MAXLINE];
	int		ftp_code;

	ftp_send_info(sock_clit, "QUIT");
	ftp_code = getreply(sock_clit, line, sizeof(line));
	usr_cprintf("%s\r\n", line);
	ftp_runningflg = FALSE;
	return FTP_OK;
}

static Status ftp_do_mkdir(SOCKET sock_clit, const char *args)
{
	char	line[MAXLINE], str[MAX_CHAR_LINE+3], *p = NULL;
	int		ftp_code;

	p = (char *) args;
	if (NULL == args) {
		usr_cprintf("(directory-name) ");
		p = usr_cgetline(str, MAX_CHAR_LINE);
		if ('\0' == *p) {
			usr_cprintf("usage: mkdir directory-name\r\n");
			return FTP_ERR;
		}
	}

	ftp_send_info(sock_clit, "MKD %s", p);
	ftp_code = getreply(sock_clit, line, sizeof(line));
	usr_cprintf("%s\r\n", line);
	if (ftp_code != 257)
		return FTP_ERR;
	return FTP_OK;
}

static Status ftp_do_rmdir(SOCKET sock_clit, const char *args)
{
	char	line[MAXLINE], str[MAX_CHAR_LINE+3], *p = NULL;
	int		ftp_code;

	p = (char *) args;
	if (NULL == args) {
		usr_cprintf("(directory-name) ");
		p = usr_cgetline(str, MAX_CHAR_LINE);
		if ('\0' == *p) {
			usr_cprintf("usage: rmdir directory-name\r\n");
			return FTP_ERR;
		}
	}

	ftp_send_info(sock_clit, "RMD %s", p);
	ftp_code = getreply(sock_clit, line, sizeof(line));
	usr_cprintf("%s\r\n", line);
	if (ftp_code != 257)
		return FTP_ERR;
	return FTP_OK;
}

static Status ftp_do_dele(SOCKET sock_clit, const char *args)
{
	char	line[MAXLINE], str[MAX_CHAR_LINE+3], *p = NULL;
	int		ftp_code;

	p = (char *) args;
	if (NULL == args) {
		usr_cprintf("(remote-file) ");
		p = usr_cgetline(str, MAX_CHAR_LINE);
		if ('\0' == *p) {
			usr_cprintf("usage: delete remote-file\r\n");
			return FTP_ERR;
		}
	}

	ftp_send_info(sock_clit, "DELE %s", p);
	ftp_code = getreply(sock_clit, line, sizeof(line));
	usr_cprintf("%s\r\n", line);
	if (ftp_code != 200)
		return FTP_ERR;
	return FTP_OK;
}

static Status ftp_do_cd(SOCKET sock_clit, const char *args)
{
	char	line[MAXLINE], str[MAX_CHAR_LINE+3], *p = NULL;
	int		ftp_code;

	p = (char *) args;
	if (NULL == args) {
		usr_cprintf("(remote-directory) ");
		p = usr_cgetline(str, MAX_CHAR_LINE);
		if ('\0' == *p) {
			usr_cprintf("usage: cd remote-directory\r\n");
			return FTP_ERR;
		}
	}

	ftp_send_info(sock_clit, "CWD %s", p);
	ftp_code = getreply(sock_clit, line, sizeof(line));
	usr_cprintf("%s\r\n", line);
	if (ftp_code != 250)
		return FTP_ERR;
	return FTP_OK;
}

static Status ftp_do_size(SOCKET sock_clit, const char *args)
{
	char	line[MAXLINE], str[MAX_CHAR_LINE+3], *p = NULL;
	int		ftp_code;

	p = (char *) args;
	if (NULL == args) {
		usr_cprintf("(filename) ");
		p = usr_cgetline(str, MAX_CHAR_LINE);
		if ('\0' == *p) {
			usr_cprintf("usage: size filename\r\n");
			return FTP_ERR;
		}
	}

	ftp_send_info(sock_clit, "SIZE %s", p);
	ftp_code = getreply(sock_clit, line, sizeof(line));
	usr_cprintf("%s\r\n", line);
	if (ftp_code != 213)
		return FTP_ERR;
	return FTP_OK;
}

static Status ftp_do_pasv(SOCKET sock_clit, const char *args)
{
	char			line[MAXLINE], pasv_ip_addr[16], *p = NULL;
	int				comma_counter, i;
	unsigned short	pasv_port, tmp;
	SOCKADDR_IN		serv_addr_in;


	ftp_send_info(sock_clit, "PASV");
	getreply(sock_clit, line, sizeof(line));
	usr_cprintf("%s\r\n", line);
	p = strchr(line, '(');
	for (++p, comma_counter = i = 0; comma_counter < 4; ++p, ++i) {
		if (',' == *p) {
			*p = '.';
			++comma_counter;
		}
		pasv_ip_addr[i] = *p;
	}
	pasv_ip_addr[i-1] = '\0';
	pasv_port = 0;
	while (*p != ',')
		pasv_port = pasv_port*10 + (*p++ - '0');
	pasv_port *= 256;
	++p;
	tmp = 0;
	while (*p != ')')
		tmp = tmp*10 + (*p++ - '0');
	pasv_port += tmp;
		// connect to ftp server on the passive port
	sock_pasv = Socket(AF_INET, SOCK_STREAM, 0);
	memset(&serv_addr_in, 0, sizeof(SOCKADDR_IN));
	serv_addr_in.sin_family		= AF_INET;
	serv_addr_in.sin_port		= htons(pasv_port);
	serv_addr_in.sin_addr.S_un.S_addr = inet_addr(pasv_ip_addr);
	Connect(sock_pasv, (SA *) &serv_addr_in, sizeof(SOCKADDR));
	return FTP_OK;
}

static Status ftp_do_list(SOCKET sock_clit, const char *args)
{
	char	line[MAXLINE], *p = NULL;
	char	buff[BUFSIZE];
	int		ftp_code, n;

	ftp_do_pasv(sock_clit, NULL);
	if (args != NULL) {
		p = line;
		while (*args != ' ' && *args != '\0')
			*p++ = *args++;
		*p = '\0';
		ftp_send_info(sock_clit, "LIST %s", line);
	} else
		ftp_send_info(sock_clit, "LIST");
	getreply(sock_clit, line, sizeof(line));
	usr_cprintf("%s\r\n", line);
		// begin to recv data through sock_pasv
	for ( ; ; ) {
		if (0 == (n = Recv(sock_pasv, buff, sizeof(buff), 0)))
			break;
		buff[n] = '\0';
		usr_cprintf(buff);
	}
	ftp_code = getreply(sock_clit, line, sizeof(line));
	usr_cprintf("%s\r\n", line);
	closesocket(sock_pasv);
	if (ftp_code != 226)
		return FTP_ERR;
	return FTP_OK;
}

static Status ftp_do_nlst(SOCKET sock_clit, const char *args)
{
	char	file_dir_name[MAX_PATH], *p = NULL, *q = NULL;
	int		i;

	p = skip_blanks(args);
	if (NULL == p || '\0' == *p) {
		ftp_do_list(sock_clit, NULL);
		return FTP_OK;
	}
	for ( ; ; ) {
		i = 0;
		while (*p != ' ' && *p != '\t' && *p != '\0')
			file_dir_name[i++] = *p++;
		file_dir_name[i] = '\0';
		usr_cprintf("file_dir_name is: %s\r\n", file_dir_name);
		ftp_do_list(sock_clit, file_dir_name);
		if ('\0' == *p)
			break;
		else {
			q = p;
			p = skip_blanks(q);
			if ('\0' == *p)
				break;
		}
	}
	return FTP_OK;
}

static Status ftp_do_type(SOCKET sock_clit, const char *args)
{
	char	line[MAXLINE], *p = NULL;
	char	tmptype;
	int		ftp_code;

	if (NULL == args) {
		usr_cprintf("Using %s mode to transfer file.\r\n",
			TYPE_A == ftp_type ? "ascii" : "binary");
		return FTP_OK;
	}
	p = line;
	while (*args != ' ' && *args != '\0')
		*p++ = *args++;
	*p = '\0';
	if (0 == strcmp(line, "ascii")) {
		tmptype = 'A';
		ftp_type = TYPE_A;
	} else {
		tmptype = 'I';
		ftp_type = TYPE_I;
	}
	ftp_send_info(sock_clit, "TYPE %c", tmptype);
	ftp_code = getreply(sock_clit, line, sizeof(line));
	usr_cprintf("%s\r\n", line);
	if (ftp_code != 200)
		return FTP_ERR;
	return FTP_OK;
}

static Status ftp_do_ascii(SOCKET sock_clit, const char *args)
{
	return ftp_do_type(sock_clit, "ascii");
}

static Status ftp_do_bin(SOCKET sock_clit, const char *args)
{
	return ftp_do_type(sock_clit, "binary");
}

static Status ftp_do_put(SOCKET sock_clit, const char *args)
{
	char	line[MAXLINE], str[MAX_CHAR_LINE+3],
			*p = NULL, *q = NULL;
	char	local_file[MAX_PATH], remote_file[MAX_PATH];
	char	buff[BUFSIZE];
	int		ftp_code, n;
	long	bytes;
	FILE	*fp = NULL;
	LARGE_INTEGER	start_count, end_count, freq;
	double	s, bs;

	p = (char *) args;
	if (NULL == args) {
		usr_cprintf("(local-file) ");
		p = usr_cgetline(str, MAX_CHAR_LINE);
		if ('\0' == *p) {
			usr_cprintf("usage: put local-file remote-file\r\n");
			return FTP_ERR;
		}
		strcpy(local_file, p);

		usr_cprintf("(remote-file) ");
		p = usr_cgetline(str, MAX_CHAR_LINE);
		if ('\0' == *p) {
			usr_cprintf("usage: put local-file remote-file\r\n");
			return FTP_ERR;
		}
		strcpy(remote_file, p);
	} else {
		p = local_file;
		while (*args != ' ' && *args != '\0')
			*p++ = *args++;
		*p = '\0';
		if (' ' == *args) {
			q = (char *) args;
			args = skip_blanks(q);
			if ('\0' == *args)
				strcpy(remote_file, local_file);
			else {
				p = remote_file;
				while (*args != ' ' && *args != '\0')
					*p++ = *args++;
				*p = '\0';
			}
		} else
			strcpy(remote_file, local_file);
	}
	usr_cprintf("local: %s remote: %s\r\n", local_file, remote_file);
	fp = fopen(local_file, 
		TYPE_A == ftp_type ? "r" : "rb");
	if (NULL == fp) {
		std_err_sys("local: %s", local_file);
		return FTP_ERR;
	}

	ftp_do_pasv(sock_clit, NULL);
	ftp_send_info(sock_clit, "STOR %s", remote_file);
	ftp_code = getreply(sock_clit, line, sizeof(line));
	usr_cprintf("%s\r\n", line);
	if (ftp_code != 150)
		return FTP_ERR;
		// begin to transfer file
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&start_count);
	bytes = 0;
	if (TYPE_I == ftp_type) {
		while (!feof(fp)) {
			n = fread(buff, sizeof(char), sizeof(buff), fp);
			bytes += n;
			Sendn(sock_pasv, buff, n, 0);
		}
	} else {
		char	tmpbuff[BUFSIZE];
		register int i, k;
		volatile int sz;

		usr_cprintf("In ascii mode...\r\n");
		while ((sz = fread(tmpbuff, sizeof(char), sizeof(tmpbuff)/2, fp)) > 0) {
			for (i = k = 0; i < sz; ++i) {
				if ('\n' == tmpbuff[i]) {
					++bytes;
					buff[k++] = '\r';
				}
				buff[k++] = tmpbuff[i];
				++bytes;
			}
			Sendn(sock_pasv, buff, k, 0);
		}
	}
	if (ferror(fp))
		FTP_DEBUG_MSG("Something may be wrong during putting.\r\n");
	fclose(fp);
	closesocket(sock_pasv);
	QueryPerformanceCounter(&end_count);
	ftp_code = getreply(sock_clit, line, sizeof(line));
	usr_cprintf("%s\r\n", line);
	s = (double) (end_count.QuadPart - start_count.QuadPart)/freq.QuadPart;
	bs = bytes / (s < 0.00000000001 ? 1 : s);
	usr_cprintf("%ld bytes transferred in %.3g secs(%.2g kbytes/s)\r\n",
		bytes, s, bs/1024.0);
	if (ftp_code != 226)
		return FTP_ERR;
	return FTP_OK;
}

static Status ftp_do_mput(SOCKET sock_clit, const char *args)
{
	char	str[MAX_CHAR_LINE+3], filename[MAX_PATH],
			*p = NULL, *q = NULL;
	int		ch, i;

	p = skip_blanks(args);
	if (NULL == p || '\0' == *p) {
		usr_cprintf("(local-files) ");
		p = usr_cgetline(str, MAX_CHAR_LINE);
		if ('\0' == *p) {
			usr_cprintf("usage: mput local-files\r\n");
			return FTP_ERR;
		}
	}
	for ( ; ; ) {
		i = 0;
		while (*p != ' ' && *p != '\t' && *p != '\0')
			filename[i++] = *p++;
		filename[i] = '\0';
		usr_cprintf("mput %s? ", filename);
		ch = usr_getche();
		if (ch != 'y' && ch != 'Y' && ch != KEY_ENTER) {
			usr_cprintf("\r\n");
			return FTP_OK;
		}
		usr_cprintf("\r\n");
		ftp_do_put(sock_clit, filename);
		if ('\0' == *p)
			break;
		else {
			q = p;
			p = skip_blanks(q);
			if ('\0' == *p)
				break;
		}
	}
	return FTP_OK;
}

static Status ftp_do_get(SOCKET sock_clit, const char *args)
{
	char	line[MAXLINE], str[MAX_CHAR_LINE+3],
			*p = NULL, *q = NULL;
	char	local_file[MAX_PATH], remote_file[MAX_PATH];
	char	buff[BUFSIZE];
	int		ftp_code, n;
	long	bytes;
	FILE	*fp = NULL;
	LARGE_INTEGER	start_count, end_count, freq;
	double	s, bs;

	p = (char *) args;
	if (NULL == args) {
		usr_cprintf("(remote-file) ");
		p = usr_cgetline(str, MAX_CHAR_LINE);
		if ('\0' == *p) {
			usr_cprintf("usage: get remote-file local-file\r\n");
			return FTP_ERR;
		}
		strcpy(remote_file, p);

		usr_cprintf("(local-file) ");
		p = usr_cgetline(str, MAX_CHAR_LINE);
		if ('\0' == *p) {
			usr_cprintf("usage: get remote-file [local-file]\r\n");
			return FTP_ERR;
		}
		strcpy(local_file, p);
	} else {
		p = remote_file;
		while (*args != ' ' && *args != '\0')
			*p++ = *args++;
		*p = '\0';
		if (' ' == *args) {
			q = (char *) args;
			args = skip_blanks(q);
			if ('\0' == *args)
				strcpy(local_file, remote_file);
			else {
				p = local_file;
				while (*args != ' ' && *args != '\0')
					*p++ = *args++;
				*p = '\0';
			}
		} else
			strcpy(local_file, remote_file);
	}
	usr_cprintf("remote-file: %s local-file: %s\r\n", remote_file, local_file);
	ftp_do_pasv(sock_clit, NULL);
	ftp_send_info(sock_clit, "RETR %s", remote_file);
	ftp_code = getreply(sock_clit, line, sizeof(line));
	usr_cprintf("%s\r\n", line);
	fp = fopen(local_file, 
		TYPE_A == ftp_type ? "w" : "wb");
	if (ftp_code != 150 || NULL == fp) {
		if (NULL == fp)
			std_err_sys("local: %s", local_file);
		else
			fclose(fp);
		closesocket(sock_pasv);
		return FTP_ERR;
	}
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&start_count);
	bytes = 0;
	if (TYPE_I == ftp_type) {
		for ( ; ; ) {		
			if (0 == (n = Recv(sock_pasv, buff, sizeof(buff), 0)))
				break;
			bytes += n;
			fwrite(buff, sizeof(char), n, fp);
		}		
	} else {
		char	ch;
		volatile int lf_count;

		lf_count = 0;
		while (1 == Recv1(sock_pasv, &ch)) {
			if ('\n' == ch)
				++lf_count;
			while ('\r' == ch) {
				++bytes;
				Recv1(sock_pasv, &ch);
				if (ch != '\n') {
					putc('\r', fp);
					if ('\0' == ch) {
						++bytes;
						goto conti;
					}
					if (EOF == ch)
						goto conti;
				}
			}
			putc(ch, fp);
			++bytes;
conti:
		NULL;
		}
		if (lf_count) {
			sprintf(line, "WARNING! %d bare linefeeds received in ASCII mode.\r\n"
					"\tFile may not have transferred correctly.", lf_count);
			FTP_DEBUG_MSG(line);
		}
		if (ferror(fp) != 0) {
			sprintf(line, "WARNING! remote-file \"%s\" received abnormally: %s\n",
					remote_file, strerror(errno));
			FTP_DEBUG_MSG(line);
		}
	}
	fclose(fp);
	closesocket(sock_pasv);
	QueryPerformanceCounter(&end_count);
	ftp_code = getreply(sock_clit, line, sizeof(line));
	usr_cprintf("%s\r\n", line);
	s = (double) (end_count.QuadPart - start_count.QuadPart)/freq.QuadPart;
	bs = bytes / (s < 0.00000000001 ? 1 : s);
	usr_cprintf("%ld bytes received in %.3g secs(%.2g kbytes/s)\r\n",
		bytes, s, bs/1024.0);
	if (ftp_code != 226)
		return FTP_ERR;
	return FTP_OK;
}

static Status ftp_do_mget(SOCKET sock_clit, const char *args)
{
	char	str[MAX_CHAR_LINE+3], filename[MAX_PATH],
			*p = NULL, *q = NULL;
	int		ch, i;

	p = skip_blanks(args);
	if (NULL == p || '\0' == *p) {
		usr_cprintf("(remote-files) ");
		p = usr_cgetline(str, MAX_CHAR_LINE);
		if ('\0' == *p) {
			usr_cprintf("usage: mget remote-files\r\n");
			return FTP_ERR;
		}
	}
	for ( ; ; ) {
		i = 0;
		while (*p != ' ' && *p != '\t' && *p != '\0')
			filename[i++] = *p++;
		filename[i] = '\0';
		usr_cprintf("mget %s? ", filename);
		ch = usr_getche();
		if (ch != 'y' && ch != 'Y' && ch != KEY_ENTER) {
			usr_cprintf("\r\n");
			return FTP_OK;
		}
		usr_cprintf("\r\n");
		ftp_do_get(sock_clit, filename);
		if ('\0' == *p)
			break;
		else {
			q = p;
			p = skip_blanks(q);
			if ('\0' == *p)
				break;
		}
	}
	return FTP_OK;
}

static Status ftp_do_mdele(SOCKET sock_clit, const char *args)
{
	char	str[MAX_CHAR_LINE+3], filename[MAX_PATH],
			*p = NULL, *q = NULL;
	int		ch, i;

	ftp_do_nlst(sock_clit, args);
	p = skip_blanks(args);
	if (NULL == p || '\0' == *p) {
		usr_cprintf("(remote-files) ");
		p = usr_cgetline(str, MAX_CHAR_LINE);
		if ('\0' == *p) {
			usr_cprintf("usage: mget remote-files\r\n");
			return FTP_ERR;
		}
	}
	for ( ; ; ) {
		i = 0;
		while (*p != ' ' && *p != '\t' && *p != '\0')
			filename[i++] = *p++;
		filename[i] = '\0';
		usr_cprintf("mdelete %s? ", filename);
		ch = usr_getche();
		if (ch != 'y' && ch != 'Y' && ch != KEY_ENTER) {
			usr_cprintf("\r\n");
			return FTP_OK;
		}
		usr_cprintf("\r\n");
		ftp_do_dele(sock_clit, filename);
		if ('\0' == *p)
			break;
		else {
			q = p;
			p = skip_blanks(q);
			if ('\0' == *p)
				break;
		}
	}
	return FTP_OK;
}

static Status ftp_do_help(SOCKET sock_clit, const char *args)
{
	usr_cprintf("%-8s   %-8s   %-8s   %-8s\r\n", "!",      "?",      "cd",     "ls");
	usr_cprintf("%-8s   %-8s   %-8s   %-8s\r\n", "nlist",  "delete", "mdelete","ascii");
	usr_cprintf("%-8s   %-8s   %-8s   %-8s\r\n", "binary", "type",   "size",   "put");
	usr_cprintf("%-8s   %-8s   %-8s   %-8s\r\n", "mput",   "get",    "mget",   "mkdir");
	usr_cprintf("%-8s   %-8s   %-8s   %-8s\r\n", "rmdir",  "system", "passive","help");
	usr_cprintf("%-8s   %-8s\r\n",               "quit",   "bye");
	usr_cprintf("and so on...\r\n");
	return FTP_OK;
}

Status ftp_do_loop(SOCKET sock_clit)
{
	char	line[MAXLINE], str[MAX_CHAR_LINE+3],
			*p = NULL, *q = NULL;
	int		i, k, possible_cmds;
	char	cmd[CMD_LEN];

	getreply(sock_clit, line, sizeof(line));
	usr_cprintf("%s\r\n", line);
	ftp_do_user(sock_clit, NULL);
	ftp_do_pass(sock_clit, NULL);
	ftp_do_syst(sock_clit, NULL);
		// do cmd loop
	for ( ; ; ) {
		usr_cprintf("ftp> ");
		p = usr_cgetline(str, MAX_CHAR_LINE);
		q = p;
		p = skip_blanks(q);
		if ('\0' == *p)
			continue;
			// to check whether it is a shell cmd
		if ('!' == *p) {
			do_shell(p+1);
			continue;
		}
		i = 0;
			// get cmd from the input line
		while (*p != ' ' && *p != '\0')
			cmd[i++] = *p++;
		cmd[i] = '\0';
		possible_cmds = 0;
		for (i = 0; ftp_cmds[i].cmd != NULL; ++i) {
			if (strlen(cmd) > 0 &&
				(0 == strncmp(cmd, ftp_cmds[i].cmd, strlen(cmd)))) {
				++possible_cmds;
				k = i;
			}
		}
			// drunk... sorry
		if (possible_cmds != 1) {
			usr_cprintf("?Ambiguous command\r\n");
			continue;
		}
		else {
			if (!ftp_cmds[k].may_have_argsflg)
				p = NULL;
			else {
				q = p;
				p = skip_blanks(q);
				if ('\0' == *p)
					p = NULL;
			}
			ftp_cmds[k].cmd_handler(sock_clit, p);
		}
		if (!ftp_runningflg)
			break;
	}
	usr_getch();
	closesocket(sock_clit);
	WSACleanup();
	return FTP_OK;
}
