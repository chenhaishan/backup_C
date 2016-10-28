#include "myconio.h"
#include <string.h>
#include "dxyh.h"
#include "login.h"
#include "record.h"
#include "error.h"

static void init_login_backg(void);

/*
 * initftp —— 用默认值初始化ftp
 */
void initftp(void)
{
	ftp_recordflg	= FALSE;	// 默认关闭日志
	ftp_debugflg	= FALSE;	// 默认不显示调试信息
	ftp_runningflg	= TRUE;		// TRUE表示ftp已开始运行
	logfile[0] = '\0';			// 存放日志文件名
	strcpy(ip_addr, "127.0.0.1"); // 初始化远程ftp服务器的IP地址
	port = SERV_PORT;			// 远程ftp服务器端口
}

/*
 *  config —— 配置ftp服务器
 */
Status config(void)
{
	char	str[MAX_CHAR_LINE+3], *p = NULL;
	int		ch;

	window(1, 1, 80, 25);
	textattr(LIGHTGRAY);
	clrscr();
	usr_cprintf("Do you want to load configure file?(y/n): ");
	ch = usr_getche();	// 回显输入一个字符后立即返回
	if (KEY_ENTER == ch || 'y' == ch || 'Y' == ch) {
		usr_cprintf("\n\r\nSpecify the file[default:\"ftp-configure.ini\"]: ");
		p = usr_cgetline(str, MAX_CHAR_LINE);
			// 用配置文件初始化ftp客户端
		load_configure('\0' == *p ? "ftp-configure.ini" : p);
	}
	else {
		usr_cprintf("\n\rInput IP(such as: 192.168.0.1) : ");
		p = usr_cgetline(str, MAX_CHAR_LINE);
		strcpy(ip_addr, p);		// 输入远程服务器IP地址
		usr_cprintf("Input port: ");
		usr_cscanf("%u", &port);// 输入远程服务器ftp端口
		usr_cprintf("Turn into debug mode?(y/n) : ");
		ch = usr_getche();
		if ('\n' == ch || 'y' == ch || 'Y' == ch)
			ftp_debugflg = TRUE;// 打开调试模式
		usr_cprintf("\n\rEnable logfile?(y/n) : ");
		ch = usr_getche();		// 打开日志模式
		if ('\n' == ch || 'y' == ch || 'Y' == ch) {
			ftp_recordflg = TRUE;
			usr_cprintf("\n\rPlease specify the logfile name: ");
			p = usr_cgetline(str, MAX_CHAR_LINE);	// 输入日志文件名
			strcpy(logfile, p);
		}
	}
		// 产生一些调试信息
	if (ftp_debugflg)
		usr_cprintf("\r\n\n");
	FTP_DEBUG(__FILE__, __LINE__, "IP is: %s\n\r", ip_addr);
	FTP_DEBUG(__FILE__, __LINE__, "port = %u\n\r", port);
	FTP_DEBUG(__FILE__, __LINE__, "tabsize = %i\n\r", tabstop);
		// 若为日志模式则打开日志文件
	if (ftp_recordflg) {
		FTP_DEBUG(__FILE__, __LINE__, "Record mode is on, logfile is: %s\n\r", logfile);
		logfp = log_open(logfile, LOG_O_DEFAULT);
			// 写入一些信息到日志文件
		log_msg(logfp, LVL_INFO, "Remote IP is: %s", ip_addr);
		log_msg(logfp, LVL_INFO, "Port is: %u", port);
	}
	usr_cprintfcolorxy(25, 15, LIGHTRED, "Press any key to continue...");
	usr_getch();
	return FTP_OK;
}

/*
 * get_word_form_str —— 从字符串中提取一个单词
 */
Status get_word_form_str(char *str, char line[], int maxlen)
{
	const char	*p = NULL;
	char		*q = NULL;
	int			counter;

	if (NULL == str || NULL == line || maxlen <= 0)
		return FTP_ERR;
		// 跳过前导空格或制表符
	p = skip_blanks(str);
	q = line;
	counter = 0;
	while (*p != ' ' && *p != '\n' && *p != '\t' && *p != '\0') {
		if (++counter >= maxlen)
			break;
		*q++ = *p++;
	}
	*q = '\0';
	return FTP_OK;
}

/*
 * load_configure —— 用配置文件初始化此ftp客户端
 */
Status load_configure(const char *configure_file)
{
	FILE	*fp = NULL;
	char	line[MAXLINE], str[MAX_CHAR_LINE],
			*p = NULL, *q = NULL;
	int		index;
	bool	quotation_markflg;
	
		// 打开配置文件
	fp = fopen(configure_file, "r");
	if (NULL == fp) {
		std_err_msg("Open configure file error.");
		return FTP_ERR;
	}
	/* 
	 * 以#开头的行为注释行。
	 * 配置项的标准形式，例如：[REMOTE = 10.6.173.225]，
	 * 其中的10.6.173.225（也就是具体内容）也可以用双引号引起来。
	 *（具体见配置文件）
	 */
	while (fgets(line, sizeof(line), fp) != NULL) {
		p = skip_blanks(line);
		if ('#' == *p || '\n' == *p)
			continue;
		if (*p != '[') {
			std_err_msg("Syntax error occurred in configure file: Missing '['.");
			return FTP_ERR;
		}
		get_word_form_str(++p, line, sizeof(line));
		q = p + strlen(line);
		p = skip_blanks(q);
		if (*p != '=') {
			std_err_msg("Missing operator '=', -_-!");
			return FTP_ERR;
		}
		q = ++p;
		p = skip_blanks(q);
		quotation_markflg = FALSE;
		if ('\"' == *p) {
			quotation_markflg = TRUE;
			++p;
		}
		index = 0;
		while (*p != '\n' && *p != '\0' 
				&& ((quotation_markflg && *p != '\"') || (!quotation_markflg && *p != ']'))) {
			if (0 == strncmp(line, "REMOTE", 6))
				ip_addr[index++] = *p++;
			else {
				if ((0 == strncmp(line, "RECORD", 6)) &&
					((',' == *p) || (' ' == *p)))
					break;
				str[index++] = *p++;
			}
		}
		if (0 == strncmp(line, "REMOTE", 6))
			ip_addr[index] = '\0';
		else {
			str[index] = '\0';
			if ((0 == strncmp(line, "DEBUG", 5)) &&
				(0 == stricmp(str, "yes") || (0 == stricmp(str, "ON"))))
				ftp_debugflg = TRUE;
			else if (0 == strncmp(line, "PORT", 4)) {
				port = atoi(str);
				if (port <= 0)
					port = SERV_PORT;
			}
			else if (0 == strncmp(line, "TABSIZE", 7)) {
				tabstop = atoi(str);
				if (tabstop <= 0)
					tabstop = TABSIZE;
			}
			else if ((0 == strncmp(line, "RECORD", 6)) &&
				(0 == stricmp(str, "yes") || (0 == stricmp(str, "ON")))) {
				ftp_recordflg = TRUE;
				q = p;
				if (NULL == (p = strstr(q, "LOGFILE"))) {
					err_msg("Missing referance \"LOGFILE\".");
					break;
				}
				q = p + 7;
				p = skip_blanks(q);
				if (*p != '=') {
					std_err_msg("Missing operator '=', -_-!");
					return FTP_ERR;
				}
				q = ++p;
				p = skip_blanks(q);
				quotation_markflg = FALSE;
				if ('\"' == *p) {
					quotation_markflg = TRUE;
					++p;
				}
				index = 0;
				while (*p != '\n' && *p != '\0'
					&& ((quotation_markflg && *p != '\"') || (!quotation_markflg && *p != ']'))) {
					logfile[index++] = *p++;
				}
				logfile[index] = '\0';
			}
		}
			// 若存在不致命的语法错误则报告一下，然后继续
		if ((quotation_markflg && (*p != '\"' || *(p+1) != ']')) ||
			(!quotation_markflg && *p != ']' && *p != ',')) {
			FTP_DEBUG_MSG("Syntax error occurred in configure file: Missing '\"' or ']'.");
			FTP_DEBUG_MSG("You can still go on...\n\r");
		}
	}
	Fclose(fp);
	return 0;
}

/*
 * init_login_backg —— 初始化登录界面
 */
static void init_login_backg(void)
{
	int		x, y;

	clrscr();
	textcolor(LIGHTGREEN);
	window(3, 4, 50, 9);
	usr_cprintf("*** *** ***   ** *   *** *** *   * ***\r\n");
	usr_cprintf("*    *  *  * *   *    *  *   **  *  * \r\n");
	usr_cprintf("**   *  ***  *   *    *  **  * * *  * \r\n");
	usr_cprintf("*    *  *    *   *    *  *   *  **  * \r\n");
	usr_cprintf("*    *  *     **  ** *** *** *   *  * \r\n");
	textcolor(WHITE);
	window(44, 3, 79, 24);
	usr_cprintf("Note: \r\n");
	usr_cprintf("\n  This is a simple ftp client tool,\r\n");
	usr_cprintf("there are sth you can do: \r\n");
	usr_cprintf("\n   1. login\r\n");
	usr_cprintf("\n   2. config\r\n");
	usr_cprintf("\n   3. do ftp stuff\r\n");
	usr_cprintf("\n   4. type '!' to enter shell\r\n");
	usr_cprintf("\n  Hope to be useful...\r\n");
	usr_cprintf("\n\n\n  Author: dengxiayehu\r\n");
	usr_cprintf("  Mail: dengxiayehu@yeah.net\r\n");
	usr_cprintf("  Date: 2010 - 11, at JUST\r\n");
	window(1, 1, 80, 25);
	draw_vertical_line(43, 2, 24, RED, 179);
	box(4, 14, 36, 21, YELLOW+(BLUE<<4), TRUE);
	textcolor(YELLOW);
	usr_cprintfxy(1, 25, "Version: 1.1");
	textbackground(BLUE);
	window(5, 15, 35, 20);
	clrscr();
	usr_cprintfxy(3, 2, "User:  ");
	wherexy(&x, &y);
	usr_cprintfcolor(LIGHTGRAY<<4, "                ");
	draw_horizen_line(x, y+1, x+15, YELLOW+(BLUE<<4), 196);
	usr_cprintfxy(3, 5, "Pass:  ");
	wherexy(&x, &y);
	usr_cprintfcolor(LIGHTGRAY<<4, "                ");
	draw_horizen_line(x, y+1, x+15, YELLOW+(BLUE<<4), 196);
	textattr(BLACK+(LIGHTGRAY<<4));
}

/*
 * login —— 处理用户登录
 */
Status login(void)
{
	char	user[USER_LEN], passwd[PASSWD_LEN],
			str[MAX_CHAR_LINE+3];
	int		counter;

		// 用默认值初始化ftp
	initftp();
		// 初始化登录界面
	init_login_backg();
	counter = 0;
	while (counter++ < TRY_TIMES) { // TRY_TIMES次尝试登录机会，默认3
		gotoxy(10, 2);
		usr_cgetline(str, MAX_CHAR_LINE);	// 输入用户名
		strcpy(user, str+2);
		if ('#' == user[0])
			return FTP_ERR;
		str[0] = MAX_CHAR_LINE;
		input_passwdxy(10, 5, str);			// 输入密码（显'*'，用input_noechoxy不回显）
		strcpy(passwd, str+2);
		if ((0 == strcmp(user, "ftpclient")) && //用户名：ftpclient，密码：123456
			(0 == strcmp(passwd, "123456")))
			return FTP_OK;
		if ('#' == passwd[0])				//输入'#'取消登录
			return FTP_ERR;
			// 失败时弹出一个确认框，警告一下
		comfirm_box(15, 8, "Error username or password!", ERROR_TYPE);
			// 重新输入时将原来的擦除
		usr_cprintfcolorxy(10, 2, LIGHTGRAY<<4, "                ");
		usr_cprintfcolorxy(10, 5, LIGHTGRAY<<4, "                ");
	}
	return FTP_ERR;
}

/*
 * atlast —— 程序结束界面
 */
void atlast(void)
{
	window(1, 1, 80, 25);
	textattr(YELLOW);
	clrscr();
	usr_cprintfxy(25, 11, "3X for the using, bye..");
	closecursor();
	textattr(LIGHTGRAY);
	usr_cprintf("   \n\n\n");
	showcursor();
	log_msg(logfp, LVL_INFO, "Ftp client is shutdown!");
	if (ftp_recordflg)
		log_close(logfp);
	return;
}
