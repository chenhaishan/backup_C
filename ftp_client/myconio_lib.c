#include <windows.h>
#include <stdarg.h>
#include "myconio.h"

enum cColorIndex {OPT_FOREG, OPT_BACKG, OPT_ATTR};
enum cInputStyle {NORM_INPUT_STYLE, PASSWORD_INPUT_STYLE, NOECHO_INPUT_STYLE};

static int	g_curwin_left	= 1;
static int	g_curwin_top	= 1;
static int	g_curwin_right	= 80;
static int	g_curwin_bottom	= 25;
static int	g_curwin_width	= 80;
static int	g_curwin_height	= 25;
int			tabstop;
static bool g_bshowtab		= TRUE;
static HANDLE g_console;
	/* 显示模式 */
static enum text_modes g_old_screen_mode = C80;
static enum text_modes g_cur_screen_mode = C80;
	/* 背景色是否高亮 */
static bool	b_backg_high_video	= FALSE;
	/* 一个输入缓冲区 */
#define MAX_MY_INPUT_BUFFER_SIZE	1024
static char g_inputbuf[MAX_MY_INPUT_BUFFER_SIZE] = {0};
	/* 用循环队列实现, 开始队头队尾为 0 */
static int	g_front = 0, g_rear = 0;

	/* 获得颜色设置的主实现函数 */
static int get_text_color_info(enum cColorIndex cGetIndex);
	/* 设置颜色设置的主要实现函数 */
static void set_text_color_info(enum cColorIndex cSetIndex, int wCharColor);
	/* 输出字符的主要实现函数 */
static int putchar_handle(int x, int y, int ch, bool bMoveCursor);
	/* 输出格式化字符串的主要实现函数 */
static int printf_handle(int x, int y, bool bMoveCursor,
						 const char * format, va_list ap);
	/* 输入字符串的主要实现函数 */
static char *gets_handle(int x, int y, bool bMoveCursor, char *pStrBuf, enum cInputStyle cStyle);
	/* 获得某个字符的属性的主要实现函数 */
static int get_certain_char_attr_handle(int x, int y, bool bMoveCursor);
	/* 设置光标模式的主要实现函数 */
static void set_cursor_mode_handle(int size, bool bVisiable);

/*
 * initscr -- 初始化函数
 *
 * 说明: 这个函最好在程序的最开始调用
 */
void initscr(void)
{
		/* 获得全局句柄 */
	g_console = GetStdHandle(STD_OUTPUT_HANDLE);
	PERR(g_console != INVALID_HANDLE_VALUE, "GetStdHandle");

		/* 初始化窗口相关变量 */
	g_curwin_left	= 1;
	g_curwin_top	= 1;
	g_curwin_right	= 80;
	g_curwin_bottom	= 25;
	g_curwin_width	= 80;
	g_curwin_height	= 25;

		/* 制表符长度默认为4 */
	tabstop		= 4;

		/* 默认显示模式为 80 * 25, 彩色 */
	g_old_screen_mode = C80;
	g_cur_screen_mode = C80;

		/* 设置窗口及窗口 buffer 的大小 */
	resize_con_buf_and_window(80, 25);

		/* 设置显示页, 默认为 MS_DOS_OUTPUT_CP, 可以显示扩展 Ascii */
	set_codepage(MS_DOS_OUTPUT_CP);

		/* 设置控制台标题 */
	set_con_title("VC - 简单FTP客户端  [灯下野狐]");

		/* 初始化输入缓冲区 */
	memset(g_inputbuf, 0, MAX_MY_INPUT_BUFFER_SIZE);
	g_front = g_rear = 0;
	return;
}


/*
 * endwin -- 结束函数 
 *
 * 说明: 这个函最好在程序结束时调用
 */
void endwin(void)
{
		/* 也没什么, 关闭句柄 */
	CloseHandle(g_console);
}


/*
 * perr -- 输出错误信息
 * @szFilename: 由 __FILE__ 传入
 * @line: 由 __LINE__ 传入
 * @szApiName: 发生错误的函数名
 * @dwError: 错误编号, 由GetLastError()传入
 */
void perr(const char *szFileName, int line, const char *szApiName, int err)
{
	TCHAR	szTemp[1024];
	DWORD	cMsgLen;
	TCHAR	*msgBuf;
	int		iButtonPressed;

		/* 格式化错误信息 */
	sprintf(szTemp, "%s: Error %d from %s on line %d:\n",
		szFileName, err, szApiName, line);
	cMsgLen = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 40, NULL, err,
		MAKELANGID(0, SUBLANG_ENGLISH_US), (LPTSTR)&msgBuf, 1024,
		NULL);
	if (cMsgLen == 0)	/* 假如未找到对应的错误信息 */
		sprintf(szTemp + strlen(szTemp), "Unable to obtain error message text!\n"
		"%s: Error %d from %s on line %d", __FILE__,
		GetLastError(), "FormatMessage", __LINE__);
	else
		strcat(szTemp, msgBuf);
	strcat(szTemp, "\n\nContinue execution?");
	MessageBeep(MB_ICONEXCLAMATION);
	iButtonPressed = MessageBox(NULL, TEXT(szTemp), "Console API Error",
		MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2);

	if (cMsgLen > 0)
		LocalFree((HLOCAL)msgBuf);

	if (iButtonPressed == IDNO)
		exit(EXIT_FAILURE);
	return;
}


/*
 * resize_con_buf_and_window -- resize the console buffer and window
 * @xsize
 * @ysize: specifies the size of buffer and window
 *
 * Remarks: It seems that 80 * 46 is the best(if you do not want the scroll bar)
 * xsize must be smaller than 80, otherwise an error occurs.
 * Try it!
 */
void resize_con_buf_and_window(int xsize, int ysize)
{
	CONSOLE_SCREEN_BUFFER_INFO	csbi;
	bool		bSuccess;
	SMALL_RECT	srWindowRect;
	COORD		coordScreen;

	if (xsize <= 0 || xsize > 80 ||
		ysize <= 0)
		return;

	bSuccess = GetConsoleScreenBufferInfo(g_console, &csbi);
	PERR(bSuccess, "GetConsoleScreenBufferInfo");

	coordScreen = GetLargestConsoleWindowSize(g_console);
	PERR(coordScreen.X | coordScreen.Y, "GetLargestConsoleWindowSize");

	srWindowRect.Right = (SHORT) (min(xsize, coordScreen.X) - 1);
	srWindowRect.Bottom = (SHORT) (min(ysize, coordScreen.Y) - 1);
	srWindowRect.Left = srWindowRect.Top = (SHORT) 0;
		/* 确定窗口缓冲区的大小(必须大于窗口的大小) */
	coordScreen.X = (SHORT) xsize;
	coordScreen.Y = (SHORT) ysize;
		/* 发现窗口缓冲区比窗口大小大时, 先调整窗口大小, 在调整缓冲区大小 */
	if ((DWORD) csbi.dwSize.X * csbi.dwSize.Y > (DWORD) xsize * ysize) {
		bSuccess = SetConsoleWindowInfo(g_console, TRUE, &srWindowRect);
		PERR(bSuccess, "SetConsoleWindowInfo");

		bSuccess = SetConsoleScreenBufferSize(g_console, coordScreen);
		PERR(bSuccess, "SetConsoleScreenBufferSize");
	}
		/* 发现窗口缓冲区比窗口大小小时, 先调整缓冲区大小, 在调整窗口大小 */
	if ((DWORD) csbi.dwSize.X * csbi.dwSize.Y < (DWORD) xsize * ysize) {
		bSuccess = SetConsoleScreenBufferSize(g_console, coordScreen);
		PERR(bSuccess, "SetConsoleScreenBufferSize");

		bSuccess = SetConsoleWindowInfo(g_console, TRUE, &srWindowRect);
		PERR(bSuccess, "SetConsoleWindowInfo");
	}
		/* 设置窗口相关的参数 */
	g_curwin_left	= 1;		/* 当前窗口的左上角横坐标 */
	g_curwin_top		= 1;		/* 当前窗口的左上角纵坐标 */
	g_curwin_right	= xsize;	/* 当前窗口的右下角横坐标 */
	g_curwin_bottom	= ysize;	/* 当前窗口的右下角纵坐标 */
	g_curwin_width	= xsize;	/* 当前窗口的宽度 */
	g_curwin_height	= ysize;	/* 当前窗口的高度 */
	return;
}


/*
 * SetOutputCodePage -- 设置输出显示代码页, 可以传入 437 (MS-DOS),
 *                这样的话可以显示扩展的 ascii 码
 * @codepageID: 代码页代号
 */
void set_codepage(unsigned int codepageID)
{
	bool	bSuccess;

	bSuccess = SetConsoleOutputCP(codepageID);
	PERR(bSuccess, "SetConsoleOutputCP");
	return;
}


/*
 * get_codepage -- 获得当前显示页的编号(为了形式整齐才设置这个函数, 直接调用API)
 * 返回值: 当前显示页编号
 */
unsigned int get_codepage(void)
{
	return GetConsoleCP();
}


/*
 * set_con_title -- 设置控制台的标题
 * @title: 标题串
 */
void set_con_title(const char *title)
{
	bool bSuccess;

	if (NULL == title)
		return;

	bSuccess = SetConsoleTitle(title);
	PERR(bSuccess, "SetConsoleTitle");
	return;
}


/*
 * pos_screen2window -- 将屏幕坐标转换为窗口坐标
 */
void pos_screen2window(int scr_x, int scr_y, int *win_x, int *win_y)
{
	if ((win_x == NULL && win_y == NULL) ||
		scr_x <= 0 || scr_x > get_conx() ||
		scr_y <= 0 || scr_y > get_cony())
		return;

	if (win_x != NULL) *win_x = scr_x - g_curwin_left  + 1;
	if (win_y != NULL) *win_y = scr_y - g_curwin_top + 1;
	return;
}


/*
 * pos_window2screen -- 将窗口坐标转换为屏幕坐标
 */
void pos_window2screen(int win_x, int win_y, int *scr_x, int *scr_y)
{
	if ((scr_x == NULL && scr_x == NULL) ||
		win_x <= 0 || win_x > g_curwin_width ||
		win_y <= 0 || win_y > g_curwin_height)
		return;

	if (scr_x != NULL) *scr_x = g_curwin_left + win_x - 1;
	if (scr_y != NULL) *scr_y = g_curwin_top + win_y - 1;
	return;
}


/*
 * Swap -- 交换两项, 可以是任意类型的变量
 * @item1: 指向交换项1的指针
 * @item2: 指向交换项2的指针
 * @size: 变量的大小(由sizeof关键字获得)
 */
void Swap(void *item1, void *item2, size_t size)
{
	void *vTmp;

	if (item1 == NULL || item2 == NULL)
		return;
		/* 在堆上分配空间给临时变量 */
	vTmp = malloc(size);
	PERR(vTmp, "malloc");
		/* 交换 */
	memcpy(vTmp, item1, size);
	memcpy(item1, item2, size);
	memcpy(item2, vTmp, size);
		/* 释放临时变量 */
	free(vTmp);
	return;
}


/*
 * gotoxy -- 移动光标
 * @x
 * @y: 目的地的坐标值(相对于当前活动窗口而言)
 *说明: 窗口左上角的坐标为(1, 1)
 */
void 	_Cdecl gotoxy			(int x, int y)
{
	COORD	coordDest;
	bool	bSuccess;
	int		sScrDestX, sScrDestY;

		/* 将窗口坐标转换为屏幕坐标(人为定, 左上角为(1, 1), 不是(0, 0)) */
	pos_window2screen(x, y, &sScrDestX, &sScrDestY);
		/* 若参数不正确就不改变位置 */
	if (x <= 0 || x > g_curwin_width ||
		y <= 0 || y > g_curwin_height)
		return;
		/* 转化为实际的屏幕坐标 */
	coordDest.X = (SHORT) (sScrDestX - 1);
	coordDest.Y = (SHORT) (sScrDestY - 1);
	bSuccess = SetConsoleCursorPosition(g_console, coordDest);
	PERR(bSuccess, "SetConsoleCursorPosition");
}

/*
 * usr_getch —— 输入一个字符（立即返回）
 */
#define VK_PAGEUP	0x21
#define VK_PAGEDOWN	0x22
int		_Cdecl usr_getch		(void)
{
	HANDLE	hStdIn;
	DWORD	dwInputMode;
	BOOL	bSuccess;
	int		ch1, ch2, retval;
	DWORD	dwInputEvents;
	INPUT_RECORD	inputBuf;
	WORD	wKeyCode;
	DWORD	wKeyState;

		/* 先看输入缓冲区中有没有字符, 有的话直接提取 */
	if (g_front != g_rear) {
		g_front = (g_front + 1) % MAX_MY_INPUT_BUFFER_SIZE;
		retval = g_inputbuf[g_front];
		return retval;
	}

	hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	PERR(hStdIn != INVALID_HANDLE_VALUE, "GetStdHandle");
		/* 获得标准输入模式 */
	bSuccess = GetConsoleMode(hStdIn, &dwInputMode);
	PERR(bSuccess, "GetConsoleMode");
		/* 将标准输入设置为单字符输入(无行缓冲, 不可见, 两者须同时取消) */
	bSuccess = SetConsoleMode(hStdIn, dwInputMode & ~(ENABLE_LINE_INPUT |
		ENABLE_ECHO_INPUT | ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT | ENABLE_PROCESSED_INPUT));
	PERR(bSuccess, "SetConsoleMode");
		/* 从输入缓冲区中取得一个字符 */

	for ( ; ; ) {
		bSuccess = ReadConsoleInput(hStdIn, &inputBuf, 1, &dwInputEvents);
		PERR(bSuccess, "ReadConsoleInput");

		if (inputBuf.Event.KeyEvent.bKeyDown && inputBuf.EventType == KEY_EVENT) {
			wKeyCode = inputBuf.Event.KeyEvent.wVirtualKeyCode;
			wKeyState = inputBuf.Event.KeyEvent.dwControlKeyState;
			if (wKeyCode == VK_SHIFT ||
				wKeyCode == VK_LSHIFT ||
				wKeyCode == VK_RSHIFT ||
				wKeyCode == VK_CONTROL ||
				wKeyCode == VK_LCONTROL ||
				wKeyCode == VK_RCONTROL ||
				wKeyCode == VK_MENU ||
				wKeyCode == VK_LMENU ||
				wKeyCode == VK_RMENU ||
				wKeyCode == VK_PAUSE ||
				wKeyCode == VK_NUMLOCK ||
				wKeyCode == VK_SCROLL ||
				wKeyCode == VK_LWIN ||
				wKeyCode == VK_RWIN ||
				wKeyCode == VK_APPS ||
				wKeyCode == VK_CAPITAL ||
				wKeyCode == VK_CLEAR ||
				wKeyCode == VK_CONVERT ||
				wKeyCode == VK_NONCONVERT ||
				wKeyCode == VK_MODECHANGE)
				continue;
			break;
		}
	}

	ch1 = inputBuf.Event.KeyEvent.uChar.AsciiChar;
	ch2 = inputBuf.Event.KeyEvent.wVirtualKeyCode;
		/* 先判定 ALT 键 */
	if ((wKeyState & RIGHT_ALT_PRESSED) ||
		(wKeyState & LEFT_ALT_PRESSED)) {
		retval = 0;
		if (0 != ch1) {
			switch (ch1) {
			case 'A':
			case 'a':
				usr_ungetch(0x1E);
				break;
			case 'B':
			case 'b':
				usr_ungetch(0x30);
				break;
			case 'C':
			case 'c':
				usr_ungetch(0x2E);
				break;
			case 'D':
			case 'd':
				usr_ungetch(0x20);
				break;
			case 'E':
			case 'e':
				usr_ungetch(0x12);
				break;
			case 'F':
			case 'f':
				usr_ungetch(0x21);
				break;
			case 'G':
			case 'g':
				usr_ungetch(0x22);
				break;
			case 'H':
			case 'h':
				usr_ungetch(0x23);
				break;
			case 'I':
			case 'i':
				usr_ungetch(0x17);
				break;
			case 'M':
			case 'm':
				usr_ungetch(0x32);
				break;
			case 'N':
			case 'n':
				usr_ungetch(0x31);
				break;
			case 'O':
			case 'o':
				usr_ungetch(0x18);
				break;
			case 'P':
			case 'p':
				usr_ungetch(0x19);
				break;
			case 'Q':
			case 'q':
				usr_ungetch(0x10);
				break;
			case 'S':
			case 's':
				usr_ungetch(0x1F);
				break;
			case 'T':
			case 't':
				usr_ungetch(0x14);
				break;
			case 'V':
			case 'v':
				usr_ungetch(0x2F);
				break;
			case 'X':
			case 'x':
				usr_ungetch(0x2D);
				break;
			/* 自己添加... */
			}
		}
		else {
			switch (ch2) {
			case VK_F1:
				usr_ungetch(0x68);
				break;
			case VK_F2:
				usr_ungetch(0x69);
				break;
			case VK_F3:
				usr_ungetch(0x6A);
				break;
			case VK_F4:
				usr_ungetch(0x6B);
				break;
			case VK_F5:
				usr_ungetch(0x6C);
				break;
			case VK_F6:
				usr_ungetch(0x6D);
				break;
			case VK_F7:
				usr_ungetch(0x6E);
				break;
			case VK_F8:
				usr_ungetch(0x6F);
				break;
			}
		}
	}	/* 再判定 CTRL 键 */
	else if ((wKeyState & RIGHT_CTRL_PRESSED) ||
			(wKeyState & LEFT_CTRL_PRESSED)) {
		retval = ch1;
	}	/* 其他情况等等 */
	else if (0 != ch1) {
		retval = ch1;
	}
	else if (!((wKeyState & RIGHT_ALT_PRESSED) ||
		(wKeyState & LEFT_ALT_PRESSED) ||
		(wKeyState & RIGHT_CTRL_PRESSED) ||
		(wKeyState & LEFT_CTRL_PRESSED) ||
		(wKeyState & SHIFT_PRESSED))) {
		retval = 0;
		switch (ch2) {
		case VK_LEFT:
			usr_ungetch(0x4B);
			break;
		case VK_UP:
			usr_ungetch(0x48);
			break;
		case VK_RIGHT:
			usr_ungetch(0x4D);
			break;
		case VK_DOWN:
			usr_ungetch(0x50);
			break;
		case VK_ESCAPE:
			retval = 0x1B;
			break;
		case VK_F1:
			usr_ungetch(0x3B);
			break;
		case VK_F2:
			usr_ungetch(0x3C);
			break;
		case VK_F3:
			usr_ungetch(0x3D);
			break;
		case VK_F4:
			usr_ungetch(0x3E);
			break;
		case VK_F5:
			usr_ungetch(0x3F);
			break;
		case VK_F6:
			usr_ungetch(0x40);
			break;
		case VK_F7:
			usr_ungetch(0x41);
			break;
		case VK_F8:
			usr_ungetch(0x42);
			break;
		case VK_F9:
			usr_ungetch(0x43);
			break;
		case VK_F10:
			usr_ungetch(0x44);
			break;
		case VK_F11:
			usr_ungetch(0x85);
			break;
		case VK_F12:
			usr_ungetch(0x86);
			break;
		case VK_HOME:
			usr_ungetch(0x47);
			break;
		case VK_PAGEUP:
			usr_ungetch(0x49);
			break;
		case VK_PAGEDOWN:
			usr_ungetch(0x51);
			break;
		case VK_END:
			usr_ungetch(0x4f);
			break;
		case VK_DELETE:
			usr_ungetch(0x53);
			break;
		case VK_INSERT:
			usr_ungetch(0x52);
			break;
		}
	}
		/* 恢复先前的输入模式 */
	bSuccess = SetConsoleMode(hStdIn, dwInputMode);
	PERR(bSuccess, "SetConsoleMode");
	return retval;
}


/*
 * usr_ungetch -- 将字符重新返回到输入缓冲区
 */
int		_Cdecl usr_ungetch		(int ch)
{
		/* 把输入缓冲区当成一个循环队列, 队列非空时才能加入 */
	if ((g_rear + 1) % MAX_MY_INPUT_BUFFER_SIZE != g_front) {
		g_rear = (g_rear + 1) % MAX_MY_INPUT_BUFFER_SIZE;
		g_inputbuf[g_rear] = ch;
	}

	return ch;
}


/*
 * bioskey -- 和原来的 bioskey 函数功能类似
 */
int	bioskey(int cmd)
{
	int		iKeyLow, iKeyHigh, key;
	int		iSpecialKeyState = 0;
	short	sTmpState;

	if (cmd != 0 && cmd != 1 && cmd != 2)
		return 0;

		/* 获得按键值 */
	if (cmd == 0) {
		iKeyLow = usr_getch();
		iKeyHigh = 0;
		if (iKeyLow == 0)
			iKeyHigh = usr_getch();
		key = (iKeyHigh << 8) + iKeyLow;
		return key;
	}	/* 判断是否有按键按下 */
	else if (cmd == 1) {
		if (kbhit()) {
			key = usr_getch();
			if (key == 0) {
				usr_ungetch(0);
				key = usr_getch();
			}
			usr_ungetch(key);
			return 1;
		}
		return 0;
	}
		/* 查询各个特殊键的状态 */
		/*  右边 SHIFT 键, 按下的话标志的最低位置1 */
	sTmpState = GetKeyState(VK_RSHIFT);
	if (get_high_byte(&sTmpState, sizeof(short)))
		set_Nth_bit(&iSpecialKeyState, sizeof(int), 1);

		/*  左边 SHIFT 键, 按下的话标志的第 2 位置1 */
	sTmpState = GetKeyState(VK_LSHIFT);
	if (get_high_byte(&sTmpState, sizeof(short)))
		set_Nth_bit(&iSpecialKeyState, sizeof(int), 2);

		/*  右边 CONTROL 键, 按下的话标志的第 3 位置1 */
	sTmpState = GetKeyState(VK_RCONTROL);
	if (get_high_byte(&sTmpState, sizeof(short)))
		set_Nth_bit(&iSpecialKeyState, sizeof(int), 3);
		/*  左边 CONTROL 键, 按下的话标志的第 3 位置1 */
	sTmpState = GetKeyState(VK_LCONTROL);
	if (get_high_byte(&sTmpState, sizeof(short)))
		set_Nth_bit(&iSpecialKeyState, sizeof(int), 3);

		/* ALT 键, 按下的话标志的第 4 为置 1 */
	sTmpState = GetKeyState(VK_MENU);
	if (get_high_byte(&sTmpState, sizeof(short)))
		set_Nth_bit(&iSpecialKeyState, sizeof(int), 4);

		/* SCROLLLOCK 键, 打开的话标志的第 5 为置 1 */
	sTmpState = GetKeyState(VK_SCROLL);
	if (get_low_byte(&sTmpState, sizeof(short)) == 1)
		set_Nth_bit(&iSpecialKeyState, sizeof(int), 5);

		/* NUMLOCK 键, 打开的话标志的第 6 为置 1 */
	sTmpState = GetKeyState(VK_NUMLOCK);
	if (get_low_byte(&sTmpState, sizeof(short)) == 1)
		set_Nth_bit(&iSpecialKeyState, sizeof(int), 6);

		/* CapsLOCK 键, 打开的话标志的第 7 位置 1 */
	sTmpState = GetKeyState(VK_CAPITAL);
	if (get_low_byte(&sTmpState, sizeof(short)) == 1)
		set_Nth_bit(&iSpecialKeyState, sizeof(int), 7);

		/* INSERT 键, 按下的话标志的第 8 为置 1 */
	sTmpState = GetKeyState(VK_INSERT);
	if (get_high_byte(&sTmpState, sizeof(short)))
		set_Nth_bit(&iSpecialKeyState, sizeof(int), 8);

	return iSpecialKeyState;
}


/*
 * set_Nth_bit -- 将变量的第 n 位置 1
 * @val: 可以是任意变量
 * @size: 变量的长度, 字节为单位
 * @index: 第 index 位
 */
void set_Nth_bit(void *val, size_t size, int index)
{
	if ((size_t) index > (size * 8) || index <= 0)
		return;

	if (size == 1)
		*(char *)val = *(char *)val | ((char) 1 << (index - 1));
	else if (size == 2)
		*(short *)val = *(short *)val | ((short) 1 << (index - 1));
	else if (size == 8)
		*(long *)val = *(long *)val | ((long) 1 << (index - 1));
	else
		*(int *)val = *(int *)val | ((int) 1 << (index - 1));
	return;
}


/*
 * reset_Nth_bit -- 将变量的第 n 位置 0
 * @val: 可以是任意变量
 * @size: 变量的长度, 字节为单位
 * @index: 第 index 位
 */
void reset_Nth_bit(void *val, size_t size, int index)
{
	if ((size_t) index > (size * 8) || index <= 0)
		return;

	if (size == 1)
		*(char *)val = *(char *)val & ~((char) 1 << (index - 1));
	else if (size == 2)
		*(short *)val = *(short *)val & ~((short) 1 << (index - 1));
	else if (size == 8)
		*(long *)val = *(long *)val & ~((long) 1 << (index - 1));
	else
		*(int *)val = *(int *)val & ~((int) 1 << (index - 1));
	return;
}


/*
 * get_low_byte -- 获得低字节的内容
 */
char get_low_byte(void *val, size_t size)
{
	if (NULL == val)
		return 0;

	if (1 == size)
		return *(char *)val;

	return *(short *)val & 0x00FF;
}


/*
 * get_high_byte -- 获得高字节的内容
 */
char get_high_byte(void *val, size_t size)
{
	if (NULL == val)
		return 0;

	if (1 == size)
		return *(char *)val;

	return ((*(short *)val & 0xFF00) >> 8);
}


/*
 * test_Nth_bit --　测试第 n 位是否为 1
 * 返回值: 为 1 返回 TRUE, 为 0 返回 FALSE.
 */
bool test_Nth_bit(void *val, size_t size, int index)
{
	long	lTmp = 0;

	if (NULL == val ||
		(size_t) index > (8 * size) || index <= 0)
		return FALSE;

	set_Nth_bit(&lTmp, sizeof(long), index);
	if (1 == size)
		return (char) (*(char *) val & (char) lTmp);
	else if (2 == size)
		return (char) (*(short *) val & (short) lTmp);
	else if (4 == size)
		return (char) (*(int *) val & (int) lTmp);
	else if (8 == size)
		return (char) (*(long *)val & (long) lTmp);
	return FALSE;
}


/*
 * flush_inputbuf -- 将输入缓冲区清空掉
 */
void flush_inputbuf(void)
{
		/* 很简单, 看作对队列操作 */
	g_front = g_rear = 0;
}

/*
 * get_conx -- 获得当前控制台活动buffer的横向大小
 * 返回值: 横向大小值
 */
int get_conx(void)
{
	CONSOLE_SCREEN_BUFFER_INFO	csbi;
	bool	bSuccess;

	bSuccess = GetConsoleScreenBufferInfo(g_console, &csbi);
	PERR(bSuccess, "GetConsoleScreenBufferInfo");
	return (csbi.dwSize.X);
}


/*
 * get_cony -- 获得当前控制台活动buffer的纵向大小
 * 返回值: 纵向大小值
 */
int get_cony(void)
{
	CONSOLE_SCREEN_BUFFER_INFO	csbi;
	bool	bSuccess;

	bSuccess = GetConsoleScreenBufferInfo(g_console, &csbi);
	PERR(bSuccess, "GetConsoleScreenBufferInfo");
	return (csbi.dwSize.Y);
}


/*
 * where_scr_x —— 获得光标的屏幕水平坐标
 */
int where_scr_x(void)
{
	int sCurScreenX;

	where_scr_xy(&sCurScreenX, NULL);
	return sCurScreenX;
}


/*
 * where_scr_y —— 获得光标的屏幕垂直坐标
 */
int where_scr_y(void)
{
	int sCurScreenY;

	where_scr_xy(NULL, &sCurScreenY);
	return sCurScreenY;
}


/*
 * where_scr_xy —— 获得光标的屏幕坐标
 */
void where_scr_xy(int *scr_x, int *scr_y)
{
	CONSOLE_SCREEN_BUFFER_INFO	csbi;
	bool	bSuccess;

	if (scr_x == NULL && scr_y == NULL)
		return;

	bSuccess = GetConsoleScreenBufferInfo(g_console, &csbi);
	PERR(bSuccess, "GetConsoleCursorInfo");
		/* 根据所获得的光标信息来通过指针返回坐标值(屏幕坐标) */
	if (scr_x != NULL) *scr_x = csbi.dwCursorPosition.X + 1;
	if (scr_y != NULL) *scr_y = csbi.dwCursorPosition.Y + 1;
	return;
}


/*
 * wherexy -- 获得当前光标的坐标
 */
void	_Cdecl wherexy			(int *x, int *y)
{
	int		scr_x, scr_y;

	if (x == NULL && y == NULL)
		return;
	where_scr_xy(&scr_x, &scr_y);
	pos_screen2window(scr_x, scr_y, x, y);
	return;
}


/*
 * wherex -- 获得当前光标的横坐标
 */
int  	_Cdecl wherex			(void)
{
	int	sCurCursorX;

	wherexy(&sCurCursorX, NULL);
	return sCurCursorX;
}


/*
 * wherey -- 获得当前光标的纵坐标
 */
int  	_Cdecl wherey			(void)
{
	int	sCurCursorY;

	wherexy(NULL, &sCurCursorY);
	return sCurCursorY;
}


/*
 * gettextcolor -- 获得当前颜色设置的前景色
 */
int		_Cdecl gettextcolor		(void)
{
	return get_text_color_info(OPT_FOREG);
}


/*
 * gettextbackground -- 获得当前颜色设置的背景色
 */
int		_Cdecl gettextbackground(void)
{
	return get_text_color_info(OPT_BACKG);
}


/*
 * gettextattr -- 获得当前的总体属性设置
 */
int		_Cdecl gettextattr		(void)
{
	return get_text_color_info(OPT_ATTR);
}


/*
 * get_text_color_info -- 获得属性设置的主要实现函数
 */
static int get_text_color_info(enum cColorIndex cGetIndex)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	bool	bSuccess;
	int		wTmpCharAttr;

	bSuccess = GetConsoleScreenBufferInfo(g_console, &csbi);
	PERR(bSuccess, "GetConsoleScreenBufferInfo");

	wTmpCharAttr = csbi.wAttributes;
	if (cGetIndex == OPT_FOREG)
		return (wTmpCharAttr & (int) 0x0000000F);
	else if (cGetIndex == OPT_BACKG)
		return ((wTmpCharAttr & (int) 0x000000F0) >> 4);
	return wTmpCharAttr;
}


/*
 * textattr -- 设置字符属性
 * @newattr: 新的属性
 */
void	_Cdecl textattr			(int newattr)
{
	set_text_color_info(OPT_ATTR, newattr);
}


/*
 * textbackground -- 设置字符的背景色
 * newcolor -- 设置的新背景色
 */
void 	_Cdecl textbackground	(int newcolor)
{
	if (!b_backg_high_video)
		newcolor &= ~(int) 0x00000008;

	set_text_color_info(OPT_BACKG, newcolor);
}


/*
 * textcolor -- 设置字符的前景色
 * newcolor -- 设置的新前景色
 */
void 	_Cdecl textcolor		(int newcolor)
{
	set_text_color_info(OPT_FOREG, newcolor);
}


/*
 * set_text_color_info -- 设置颜色等等属性的主要实现函数
 * @cSetIndex: 设置标识, 表示或是对前景色设置, 或是对背景色设置, 或是对整个属性的设置
 * @wCharColor: 设置的新的项
 */
static void set_text_color_info(enum cColorIndex cSetIndex, int wCharColor)
{
	bool	bSuccess;
	int		wTmpCharAttr, wCharOldAttr;
	DWORD	dwCharsWritten;
	COORD	coordBuf = {0, 0};

		/* 下面两种模式只有黑白两色 */
	if (g_cur_screen_mode == BW40 || g_cur_screen_mode == BW80) {
		wTmpCharAttr = (int) ((LIGHTGRAY + (BLACK << 4)) | (wCharColor & (int) 0x0000FF88));
		bSuccess = SetConsoleTextAttribute(g_console, (WORD) wTmpCharAttr);
	}
	else if (g_cur_screen_mode == MONO) {
			/* 获得当前的颜色设置 */
		wCharOldAttr = gettextattr();
			/* 若是单色模式要改变前景色 */
		if (cSetIndex == OPT_FOREG)
			wTmpCharAttr = (wCharOldAttr & (int) 0x0000FF80) | (wCharColor & (int) 0x0000000F);
		else if (cSetIndex == OPT_BACKG)
			wTmpCharAttr = (wCharOldAttr & (int) 0x0000FF0F) | ((wCharColor << 4) & (int) 0x00000080);
		else if (cSetIndex == OPT_ATTR)
			wTmpCharAttr = wCharColor & (int) 0x0000FF8F;
		bSuccess = FillConsoleOutputAttribute(g_console, (WORD) wTmpCharAttr,
							(DWORD) (get_conx() * get_cony()), coordBuf, &dwCharsWritten);
		PERR(bSuccess, "FillConsoleOutputAttribute");

		bSuccess = SetConsoleTextAttribute(g_console, (WORD) wTmpCharAttr);		
	}
	else {
			/* 假如是彩色模式, 并且是设置字符的属性, 则一股脑设置 */
		if (cSetIndex == OPT_ATTR)
			bSuccess = SetConsoleTextAttribute(g_console, (WORD) wCharColor);
		else {
				/* 先获得当前的颜色设置, 以备后面设置 */
			wCharOldAttr = gettextattr();
				/* 下面根据要求设置颜色 */
			if (cSetIndex == OPT_FOREG)
				wTmpCharAttr = (wCharOldAttr & (int) 0x0000FFF0) | (wCharColor & (int) 0x0000000F);
			else if (cSetIndex == OPT_BACKG)
				wTmpCharAttr = (wCharOldAttr & (int) 0x0000FF0F) | ((wCharColor << 4) & (int) 0x000000F0);

			bSuccess = SetConsoleTextAttribute(g_console, (WORD) wTmpCharAttr);
		}		
	}
	PERR(bSuccess, "SetConsoleTextAttribute");
	return;
}


/*
 * fill_rect_attr -- 改变指定区域字符的属性, 不改变当前的颜色设置
 * @left:
 * @top: 区域左上角
 * @right:
 * @bottom: 区域右下角
 * @char_attr: 指定的属性
 */
void fill_rect_attr(int left, int top, int right, int bottom, int char_attr)
{
	COORD	coordStart, coordEnd;
	COORD	coordBuf;
	bool	bSuccess;
	DWORD	dwFillSize, dwCharsWritten;
	int		wTmpOutputCharAttr, iOldCharAttr;

		/* 参数出错, 不填充 */
	if (left <= 0 || left > get_conx() ||
		right <= 0 || right > get_conx() ||
		top <= 0 || top > get_cony() ||
		bottom <= 0 || bottom > get_cony())
		return;

	iOldCharAttr = gettextattr();
		/* 设置填充的颜色 */
	if (g_cur_screen_mode == BW40 || g_cur_screen_mode == BW80)
		wTmpOutputCharAttr = (char_attr & (int) 0x0000FF88) | (int) 0x00000007;
	else if (g_cur_screen_mode == MONO)
		wTmpOutputCharAttr = iOldCharAttr & (int) 0x0000FF8F;
	else
		wTmpOutputCharAttr = char_attr;
	
		/* 计算改变区域的实际物理坐标 */
	coordStart.X = (SHORT) (min(left, right) - 1);
	coordStart.Y = (SHORT) (min(top, bottom) - 1);
	coordEnd.X = (SHORT) (max(left, right) - 1);
	coordEnd.Y = (SHORT) (max(top, bottom) - 1);
		/* 计算每行填充缓冲区的大小 */
	dwFillSize = (coordEnd.X - coordStart.X + 1);
	for (coordBuf = coordStart; coordBuf.Y <= coordEnd.Y; ++coordBuf.Y) {
		bSuccess = FillConsoleOutputAttribute(g_console, (WORD) wTmpOutputCharAttr, dwFillSize,
			coordBuf, &dwCharsWritten);
		PERR(bSuccess, "FillConsoleOutputAttribute");	
	}
		/* 用这种方法填充的话可以不改变先前的颜色设置 */
	return;
}


/*
 * fill_rect_char -- 以特定属性的字符填充指定区域, 不改变当前的颜色设置
 * @left:
 * @top: 区域左上角
 * @right:
 * @bottom: 区域右下角
 * @char_attr: 指定的属性
 * @cFillChar: 填充的字符
 */
void fill_rect_char(int left, int top, int right, int bottom,
				   int char_attr, int cFillChar)
{
	COORD	coordStart, coordEnd;
	COORD	coordBuf;
	bool	bSuccess;
	DWORD	dwFillSize, dwCharsWritten;
	int		wTmpOutputCharAttr, iOldCharAttr;

		/* 参数出错, 不填充 */
	if (left <= 0 || left > get_conx() ||
		right <= 0 || right > get_conx() ||
		top <= 0 || top > get_cony() ||
		bottom <= 0 || bottom > get_cony())
		return;

	iOldCharAttr = gettextattr();

	coordStart.X = (SHORT) (min(left, right) - 1);
	coordStart.Y = (SHORT) (min(top, bottom) - 1);
	coordEnd.X = (SHORT) (max(left, right) - 1);
	coordEnd.Y = (SHORT) (max(top, bottom) - 1);

		/* 设置填充的颜色 */
	if (g_cur_screen_mode == BW40 || g_cur_screen_mode == BW80)
		wTmpOutputCharAttr = (char_attr & (int) 0x0000FF88) | (int) 0x00000007;
	else if (g_cur_screen_mode == MONO)
		wTmpOutputCharAttr = iOldCharAttr & (int) 0x0000FF8F;
	else
		wTmpOutputCharAttr = char_attr;

		/* 计算每行填充缓冲区的大小 */
	dwFillSize = (coordEnd.X - coordStart.X + 1);
	for (coordBuf = coordStart; coordBuf.Y <= coordEnd.Y; ++coordBuf.Y) {		
			/* 先填充字符(颜色不一定就正确) */
		bSuccess = FillConsoleOutputCharacter(g_console, (TCHAR) cFillChar, dwFillSize,
			coordBuf, &dwCharsWritten);
		PERR(bSuccess, "FillConsoleOutputCharacter");
			/* 再来正确设置输出字符的颜色 */
		bSuccess = FillConsoleOutputAttribute(g_console, (WORD) wTmpOutputCharAttr, dwFillSize,
			coordBuf, &dwCharsWritten);
		PERR(bSuccess, "FillConsoleOutputAttribute");	
	}
	return;
}


/*
 * clrscr -- 对当前活动的窗口进行刷屏
 */
void	_Cdecl clrscr			(void)
{
	int	char_attr = gettextattr();

		/* 调用另外一个函数进行填充(注: 无论何时, 传入给fillRectangle 的坐标值都是屏幕坐标) */
	fill_rect_char(g_curwin_left, g_curwin_top, g_curwin_right, g_curwin_bottom,
		char_attr, ' ');
		/* 刷屏后将光标置为(1, 1)处 */
	gotoxy(1, 1);
	return;
}


/*
 * window -- 开辟一个新的活动窗口
 * @left:
 * @top: 窗口左上角
 * @right:
 * @bottom: 窗口右下角
 */
void 	_Cdecl window			(int left, int top, int right, int bottom)
{
			/* 参数检验 */
	if (left <= 0 || left > get_conx() ||
		right <= 0 || right > get_conx() ||
		top <= 0 || top > get_cony() ||
		bottom <= 0 || bottom > get_cony())
		return;
		/* 参数处理 */
	if (top > bottom)
		Swap(&top, &bottom, sizeof(int));
	if (left > right)
		Swap(&left, &right, sizeof(int));
		/* 设置当前获得窗口区域 */
	g_curwin_left		= left;
	g_curwin_top		= top;
	g_curwin_right	= right;
	g_curwin_bottom	= bottom;
	g_curwin_width	= right - left + 1;
	g_curwin_height	= bottom - top + 1;
		/* 设置当前光标位置 */
	gotoxy(1, 1);
	return;
}


/*
 * textmode -- 设置显示模式
 * @newmode: 新的显示模式, 具体定义见头文件
 */
void 	_Cdecl textmode			(int newmode)
{
	switch (newmode) {
	case BW40:
			/* 若原来已经是这种模式就不改变 */
		if (BW40 == g_old_screen_mode)
			return;
			/* 黑白 40 * 25, 改变窗口大小 */
		resize_con_buf_and_window(40, 25);
		g_old_screen_mode = g_cur_screen_mode;
		g_cur_screen_mode = BW40;
		break;
	case C40:
		if (C40 == g_old_screen_mode)
			return;
			/* 彩色 40 * 25, 改变窗口大小, 以下一样 */
		resize_con_buf_and_window(40, 25);
		g_old_screen_mode = g_cur_screen_mode;
		g_cur_screen_mode = C40;
		break;
	case BW80:
		if (BW80 == g_old_screen_mode)
			return;
		resize_con_buf_and_window(80, 25);
		g_old_screen_mode = g_cur_screen_mode;
		g_cur_screen_mode = BW80;	
		break;
	case C80:
		if (C80 == g_old_screen_mode)
			return;
		resize_con_buf_and_window(80, 25);
		g_old_screen_mode = g_cur_screen_mode;
		g_cur_screen_mode = C80;
		break;
	case MONO:
		if (MONO == g_old_screen_mode)
			return;
		resize_con_buf_and_window(80, 25);
		g_old_screen_mode = g_cur_screen_mode;
		g_cur_screen_mode = MONO;	
		break;
	case LASTMODE:
			/* 设置成上一次的模式 */
		textmode(g_old_screen_mode);
		break;
	default:
		break;
	}

	b_backg_high_video = FALSE;
	textattr(LIGHTGRAY + (BLACK << 4));
	clrscr();
	return;
}


/*
 * highvideo -- 前景色高亮
 *
 * 说明: 实际上 VC 是支持背景色高亮的, 下面会有个函数 backghighvideo
 */
void	_Cdecl highvideo		(void)
{
	int	wCharForeg;

	wCharForeg = gettextcolor();
	wCharForeg |= FOREGROUND_INTENSITY;
	textcolor(wCharForeg);
	return;
}


/*
 * normvideo -- 将字符的属性恢复为默认
 *
 * 说明: 一切属性, 恢复为: 黑底白字
 */
void	_Cdecl normvideo		(void)
{
	b_backg_high_video = FALSE;
	textattr(LIGHTGRAY + (BLACK << 4));
}


/*
 * gettextinfo -- 获得窗体信息
 * @r: 窗体信息结构体指针
 */
void	_Cdecl gettextinfo		(struct text_info *r)
{
	int		iTmpCharAttr;

	if (NULL == r)
		return;

	iTmpCharAttr = gettextattr();

		/* 获得当前获得窗口区域 */
	r->winleft		= g_curwin_left;
	r->wintop		= g_curwin_top;
	r->winright		= g_curwin_right;
	r->winbottom	= g_curwin_bottom;
		/* 当前颜色设置 */
	r->attribute = gettextattr();
		/* 前景色是否高亮 */
	r->normattr = iTmpCharAttr & (int) 0x00000008 ? 0 : 1;
		/* 获得当前窗口的最大值 */
	r->screenwidth	= g_curwin_width;
	r->screenheight	= g_curwin_height;
		/* 获得当前光标的位置 */
	wherexy((int *) &r->curx, (int *) &r->cury);
		/* 获得当前的显示模式 */
	r->currmode = g_cur_screen_mode;
	return;
}


/*
 * usr_putchar -- 和 putchar 相对, 输出一个字符
 * @c: 要输出的字符
 * 返回值: 输出字符的 ascii
 */
int		_Cdecl usr_putchar		(int c)
{
	return putchar_handle(0, 0, c, FALSE);
}


/*
 * usr_putch -- 和 usr_putchar 功能一样
 * @c: 要输出的字符
 * 返回值: 输出字符的 ascii
 */
int		_Cdecl usr_putch		(int c)
{
	return putchar_handle(0, 0, c, FALSE);
}


/*
 * usr_putcharxy -- 在窗口中指定位置输出字符
 * @x:
 * @y: 输出的字符坐标(窗口坐标)
 * @c: 要输出的字符
 * 返回值: 输出字符的 ascii
 */
int		_Cdecl usr_putcharxy	(int x, int y, int c)
{
	return putchar_handle(x, y, c, TRUE);
}


/*
 * putchar_handle -- 输出字符的主要实现函数
 * @x:
 * @y: 输出的字符坐标(窗口坐标)
 * @c: 要输出的字符
 * @bMoveCursor: 移动光标是否有效
 * 返回值: 输出字符的 ascii
 */
static int putchar_handle(int x, int y, int ch, bool bMoveCursor)
{
	bool	bSuccess;
	DWORD	dwCharWritten;
	COORD	coordCurPos;
	int		char_attr;
	int		scr_x, scr_y,
			win_x, win_y;
	bool	bScrollWindow = FALSE;

		/* 参数检验 */
	if (bMoveCursor && (x <= 0 || x > g_curwin_width || y <= 0 || y > g_curwin_height))
		return EOF;


		/* 获得当前的颜色设置 */
	char_attr = gettextattr();
		/* 移动光标并获得光标的当前位置 */
	if (bMoveCursor) {
		gotoxy(x, y);
		win_x = x;
		win_y = y;
	}
	else
		wherexy(&win_x, &win_y);

		/* 特殊情况先判断 */
	if (ch == '\n') {
			/* 若已到窗口的最下面一行, 则上滚窗口 */
		if (win_y == g_curwin_height)
			movetext(g_curwin_left, g_curwin_top, g_curwin_right, g_curwin_bottom,
					g_curwin_left, g_curwin_top - 1);
		else
			gotoxy(win_x, ++win_y);
		return ch;
	}

	if (ch == '\r') {
		gotoxy(1, wherey());
		return ch;
	}

	if (ch == '\b') {
		gotoxy(wherex() > 1 ? wherex() - 1 : 1, wherey());
		return ch;
	}

	if (ch == '\t' && g_bshowtab) {
		win_x += ((tabstop + 1) - (win_x % (tabstop + 1)));
		if (win_x > g_curwin_width) {
			if (win_y == g_curwin_height) {
				movetext(g_curwin_left, g_curwin_top, g_curwin_right, g_curwin_bottom,
						g_curwin_left, g_curwin_top - 1);
				gotoxy(win_x - g_curwin_width, g_curwin_height);
			}
			else
				gotoxy(win_x - g_curwin_width, wherey()+1);
		} else
			gotoxy(win_x, wherey());
		return ch;
	}
		/* 转化为屏幕坐标 */
	pos_window2screen(win_x, win_y, &scr_x, &scr_y);
		/* 转化为物理坐标 */
	coordCurPos.X = (SHORT) (scr_x - 1);
	coordCurPos.Y = (SHORT) (scr_y - 1);
		/* 输出 */
	bSuccess = FillConsoleOutputCharacter(g_console, (TCHAR) ch, 1,
		coordCurPos, &dwCharWritten);
	PERR(bSuccess, "FillConsoleOutputCharacter");
	bSuccess = FillConsoleOutputAttribute(g_console, (WORD) char_attr, 1,
		coordCurPos, &dwCharWritten);
	PERR(bSuccess, "FillConsoleOutputCharacter");

		/* 输出字符后光标"向后", 若已到窗口的最右边 */
	if (win_x == g_curwin_width) {
			/* 假如到了窗口的最下边 */
		if (win_y == g_curwin_height) {
				/* 滚动 */
			movetext(g_curwin_left, g_curwin_top, g_curwin_right, g_curwin_bottom,
					g_curwin_left, g_curwin_top - 1);
			gotoxy(1, g_curwin_height);
		}
		else
			gotoxy(1, ++win_y);
	}
	else {
			/* 一般字符只要往后一个位置 */
		gotoxy(++win_x, win_y);
	}

	return ch;
}


/*
 * movetext -- 移动屏幕区域内容到指定位置
 * @left:
 * @top: 指定区域的左上角
 * @right:
 * @bottom: 指定区域的右下角
 * @destleft: 
 * @desttop: 目标位置
 * 返回值: 成功 1, 失败 0
 */
int		_Cdecl movetext			(int left, int top, int right, int bottom,
								 int destleft, int desttop)
{
	bool		bSuccess;
	COORD		coordDest;
	CHAR_INFO	chiFill;
	int			char_attr;
	SMALL_RECT	srctScrollRect, srctClipRect;

		/* 参数检验 */
	if (left <= 0 || left > get_conx() ||
		right <= 0 || right > get_conx() ||
		top <= 0 || top > get_cony() ||
		bottom <= 0 || bottom > get_cony() ||
		destleft <=0 || destleft > get_conx() ||
		desttop <= 0 || desttop> get_cony())
		return 0;

		/* 参数处理 */
	if (top > bottom)
		Swap(&top, &bottom, sizeof(int));
	if (left > right)
		Swap(&left, &right, sizeof(int));

		/* 设置要移动的区域 */
	srctScrollRect.Left		= (SHORT) (left - 1);
	srctScrollRect.Top		= (SHORT) (top -  1);
	srctScrollRect.Right	= (SHORT) (right - 1);
	srctScrollRect.Bottom	= (SHORT) (bottom - 1);
		/* 设置剪裁区域(也就是整个活动窗口) */
	srctClipRect.Left	= (SHORT) (g_curwin_left - 1);
	srctClipRect.Top	= (SHORT) (g_curwin_top - 1);
	srctClipRect.Right	= (SHORT) (g_curwin_right - 1);
	srctClipRect.Bottom	= (SHORT) (g_curwin_bottom - 1);
		/* 设置目的地坐标 */
	coordDest.X = (SHORT) (destleft - 1);
	coordDest.Y = (SHORT) (desttop - 1);
		/* 获得当前颜色的设置 */
	char_attr = gettextattr();
		/* 设置移动过后留下的"空白"处的填充字符及属性 */
	chiFill.Char.AsciiChar = ' ';	/* 用空格填充 */
	chiFill.Attributes = (WORD) char_attr;
		/* 具体移动 */
	bSuccess = ScrollConsoleScreenBuffer(g_console, &srctScrollRect,
		&srctClipRect, coordDest, &chiFill);
	PERR(bSuccess, "ScrollConsoleScreenBuffer");
	return 1;
}


/*
 * usr_cprintf -- 和 cprintf 类似, 从当前位置输出格式串
 * @format: 格式串
 * 返回值: 返回输出的字符个数
 */
int		_Cdecl usr_cprintf		(const char *format, ...)
{
	va_list	ap;
	int		dwCharsWrittenInTotal;

	if (NULL == format)
		return 0;

		/* 处理可变参数列表 */
	va_start(ap, format);
	dwCharsWrittenInTotal = printf_handle(0, 0, FALSE, format, ap);
	va_end(ap);
	return dwCharsWrittenInTotal;
}


/*
 * usr_cprintfxy -- 从指定位置开始输出格式串
 * @x:
 * @y: 指定的位置 (窗口坐标)
 * @format: 格式串
 * 返回值: 返回输出的字符个数
 */
int		_Cdecl usr_cprintfxy		(int x, int y, const char *format, ...)
{
	va_list	ap;
	int dwCharsWrittenInTotal;

	if (x <= 0 || x > g_curwin_width ||
		y <= 0 || y > g_curwin_height ||
		NULL == format)
	return 0;

	va_start(ap, format);
	dwCharsWrittenInTotal = printf_handle(x, y, TRUE, format, ap);
	va_end(ap);
	return dwCharsWrittenInTotal;
}


/*
 * usr_cprintfcolorxy -- 在窗口中当前位置处输出指定颜色属性的格式串
 */
int		_Cdecl usr_cprintfcolor	(int attr, const char *format, ...)
{
	va_list	ap;
	int		dwCharsWrittenInTotal,
			iOldAttr;

	if (NULL == format)
		return 0;

	iOldAttr = gettextattr();
	textattr(attr);
		/* 处理可变参数列表 */
	va_start(ap, format);
	dwCharsWrittenInTotal = printf_handle(0, 0, FALSE, format, ap);
	va_end(ap);
	textattr(iOldAttr);
	return dwCharsWrittenInTotal;
}


/*
 * usr_cprintfcolorxy -- 在窗口中指定坐标处输出指定颜色属性的格式串 
 */
int		_Cdecl usr_cprintfcolorxy(int x, int y, int attr, const char *format, ...)
{
	va_list	ap;
	int		dwCharsWrittenInTotal,
			iOldAttr;

	if (x <= 0 || x > g_curwin_width ||
		y <= 0 || y > g_curwin_height || NULL == format)
		return 0;

	iOldAttr = gettextattr();
	textattr(attr);

	va_start(ap, format);
	dwCharsWrittenInTotal = printf_handle(x, y, TRUE, format, ap);
	va_end(ap);
	textattr(iOldAttr);
	return dwCharsWrittenInTotal;
}


/*
 * usr_cputs -- 输出字符串
 * @str: 要输出的字符串
 * 返回值: 返回输出的字符个数
 */
int		_Cdecl usr_cputs		(const char *str)
{
	int		dwCharsWrittenInTotal;

	if (NULL == str)
		return 0;

	dwCharsWrittenInTotal = usr_cprintf(str);
	/* usr_putchar('\n'); */	/* 原库的 cputs 在输出结束也没有输出换行符 */
	return dwCharsWrittenInTotal;
}


/*
 * printf_handle -- 输出字符串的主要函数
 * @x:
 * @y: 输出位置
 * @bMoveCursor: 是否移动光标
 * @format: 格式串
 * @ap: 可变参
 * 返回值: 返回输出的字符个数
 */
static int printf_handle(int x, int y, bool bMoveCursor,
						 const char * format, va_list ap)
{
	char	szTmpBuf[BUFSIZE], *p = NULL;
	bool	bSuccess;
	DWORD	dwCharsWritten;
	int		dwCharsIntendToWrite, dwCharsWrittenInTotal = 0;
	COORD	coordCurPos;
	int		char_attr;
	int		scr_x, scr_y,
			win_x, win_y,
			i, iIndex;
	bool	bLfOccurs = FALSE,
			bCrOccurs = FALSE,
			bBakOccurs = FALSE,
			bTabOccurs = FALSE;

		/* 参数检验 */
	if (bMoveCursor && (x <= 0 || x > g_curwin_width || y <= 0 || y > g_curwin_height))
		return EOF;

		/* 获得当前的颜色设置 */
	char_attr = gettextattr();

	if (bMoveCursor) {
		gotoxy(x, y);
		win_x = x;
		win_y = y;
	}
	else
		wherexy(&win_x, &win_y);
	vsprintf(szTmpBuf, format, ap);
	p = szTmpBuf;
	for ( ; ; ) {
				/* 转化为屏幕坐标 */
		pos_window2screen(wherex(), wherey(), &scr_x, &scr_y);
			/* 转化为实际物理坐标 */
		coordCurPos.X = (SHORT) (scr_x - 1);
		coordCurPos.Y = (SHORT) (scr_y - 1);
			/* 只能在规定的区域(活动窗口范围之内)输出, 超过就换行或滚屏 */
		dwCharsIntendToWrite = min(lstrlen(p), (g_curwin_width - wherex() + 1));
		dwCharsWrittenInTotal += dwCharsIntendToWrite;

		bLfOccurs	= FALSE,
		bCrOccurs	= FALSE,
		bBakOccurs	= FALSE,
		bTabOccurs	= FALSE;
		iIndex = 0;
		for (i = 0; i < dwCharsIntendToWrite; ++i) {
				/* 格式串中含有回车 */
			if (p[i] == '\n') {
				iIndex = i;
				bCrOccurs = TRUE;
				break;
			}	/* 格式串中含有 '\r' */
			else if (p[i] == '\r') {
				iIndex = i;
				bLfOccurs = TRUE;
				break;
			}	/* 格式串中含有 '\b' */
			else if (p[i] == '\b') {
				iIndex = i;
				bBakOccurs = TRUE;
				break;
			}	/* 格式串中含有 '\t' */
			else if (p[i] == '\t' && g_bshowtab) {
				iIndex = i;
				bTabOccurs = TRUE;
				break;
			}
		}

		if (bLfOccurs || bCrOccurs || bBakOccurs || bTabOccurs) {
			bSuccess = WriteConsole(g_console, p, iIndex, &dwCharsWritten, NULL);
			PERR(bSuccess, "WriteConsole");
			if (bLfOccurs)			usr_putchar('\r');
			else if (bCrOccurs)		usr_putchar('\n');
			else if (bBakOccurs)	usr_putchar('\b');
			else if (bTabOccurs)	usr_putchar('\t');
			p += (iIndex + 1);
			++dwCharsWrittenInTotal;
			continue;
		}
			/* 否则一串输出 */
		bSuccess = WriteConsole(g_console, p, dwCharsIntendToWrite, &dwCharsWritten, NULL);
		PERR(bSuccess, "WriteConsole");

		if (where_scr_x() > g_curwin_right) {
			if (coordCurPos.Y >= g_curwin_bottom - 1) {
					/* 滚屏用移动窗口内容来实现 */
				movetext(g_curwin_left, g_curwin_top, g_curwin_right, g_curwin_bottom,
					g_curwin_left, g_curwin_top - 1);
					/* 光标移动到窗口的左下角 */
				gotoxy(1, g_curwin_height);
			}
			else
				gotoxy(1, wherey()+1);
		}
		p += dwCharsIntendToWrite;
			/* 输出到最后了就退出显示 */
		if (*p == '\0')
			break;
	}
	return dwCharsWrittenInTotal;
}


/*
 * usr_getche -- 带回显, 输出字符
 * 返回值: 输出的字符的 ascii
 */
int		_Cdecl usr_getche		(void)
{
	int		ch;

	ch = usr_getch();
		/* 将刚刚输入的字符显示出来, 输入滚动效果由它实现的 */
	usr_putchar(ch);
	return ch;
}


/*
 * clreol -- clear end of line
 */
void	_Cdecl clreol			(void)
{
	int		scr_x, scr_y,
			win_x, win_y;
	DWORD	dwCharsWrittn;
	char	szTmp[256];
	int		len;
	COORD	coordCurPos;
	bool	bSuccess;
	int		char_attr;

		/* 获得当前的颜色设置 */
	char_attr = gettextattr();
		/* 获得当前光标的位置(窗口相关) */
	wherexy(&win_x, &win_y);
		/* 转化为屏幕坐标 */
	pos_window2screen(win_x, win_y, &scr_x, &scr_y);
	coordCurPos.X = (SHORT) (scr_x - 1);
	coordCurPos.Y = (SHORT) (scr_y - 1);
		/* 计算横向要消除的字符数 */
	len = g_curwin_width - win_x + 1;
		/* 开始清除... */
	memset(szTmp, ' ', len);
	bSuccess = WriteConsoleOutputCharacter(g_console, szTmp, len,
		coordCurPos, &dwCharsWrittn);
	PERR(bSuccess, "WriteConsoleOutputCharacter");
	bSuccess = FillConsoleOutputAttribute(g_console, (WORD) char_attr, len,
		coordCurPos, &dwCharsWrittn);
	PERR(bSuccess, "FillConsoleOutputAttribute");
	return;
}


/*
 * delline -- 删除光标所在的那行, 窗口下面的内容上滚
 */
void	_Cdecl delline			(void)
{
	int		sCurCursorX, sCurCursorY,
			scr_x, scr_y;

	wherexy(&sCurCursorX, &sCurCursorY);
		/* 假如是最后一行那就直接用空格去填充 */
	if (sCurCursorY == g_curwin_height) {
		gotoxy(1, g_curwin_height);
		clreol();
			/* 将光标移回原来的地方 */
		gotoxy(sCurCursorX, sCurCursorY);
	}
	else {
		pos_window2screen(sCurCursorX, sCurCursorY, &scr_x, &scr_y);
			/* 其他情况可以考虑用移动窗口来做 */
		movetext(g_curwin_left, scr_y + 1, g_curwin_right, g_curwin_bottom, 
			g_curwin_left, scr_y);
	}

	return;
}


/*
 * insline -- insert line 在光标当前位置插入一行, 窗口下面的内容下滚
 */
void 	_Cdecl insline			(void)
{
	int		sCurCursorX, sCurCursorY,
			scr_x, scr_y;

	wherexy(&sCurCursorX, &sCurCursorY);
		/* 假如是最后一行那就直接用空格去填充 */
	if (sCurCursorY == g_curwin_height) {
		gotoxy(1, g_curwin_height);
		clreol();
			/* 将光标移回原来的地方 */
		gotoxy(sCurCursorX, sCurCursorY);
	}
	else {
		pos_window2screen(sCurCursorX, sCurCursorY, &scr_x, &scr_y);
			/* 其他情况可以考虑用移动窗口来做 */
		movetext(g_curwin_left, scr_y, g_curwin_right,
			g_curwin_bottom, g_curwin_left, scr_y + 1);
	}
	return;
}


/*
 * usr_cgets -- 输入字符串
 * 返回值: 返回输入的字符串
 *
 * 说明: str[0] 为要输入的最大字符个数, str[1] 会返回实际输入的个数, str + 2 开始才是输入的内容
 */
char   *_Cdecl usr_cgets		(char *str)
{
	char	*p = NULL;

		/* 参数检验 */
	if (str == NULL)
		return NULL;

	p = gets_handle(0, 0, FALSE, str, NORM_INPUT_STYLE);
	return p;
}


/*
 * gets_handle -- 输入函数的主要实现函数
 * @x:
 * @y: 从哪个位置开始输入
 * @bMoveCursor: 是否移动光标
 * @pStrBuf: 输入的字符串所存之处
 * @cStyle: 输入的类型
 *
 * 说明: 输入的类型有 3 中, 1普通输入类型, 2回显'*'输入类型, 3不回显输入类型
 */
#ifndef MAX_INPUT_LEN
#define MAX_INPUT_LEN	(MAX_CHAR_LINE+3)
#endif
static char *gets_handle(int x, int y, bool bMoveCursor, char *pStrBuf, enum cInputStyle cStyle)
{	
	char	szTmpBuf[MAX_INPUT_LEN];
	char	*p;
	int		ch, len = 0, iMaxInputChars, i;
	int		win_x, win_y,
			sOriginX, sOriginY;
	bool	bFull = FALSE;

		/* 参数检验 */
	if (pStrBuf == NULL ||
		(bMoveCursor && (x <= 0 || x > get_conx() || y <= 0 || y > get_cony())))
		return NULL;

	iMaxInputChars = pStrBuf[0];
	if (iMaxInputChars < 0)
		iMaxInputChars += (MAX_INPUT_LEN - 1);

		/* 判断是否需要移动光标 */
	if (bMoveCursor) {
		gotoxy(x, y);
		sOriginX = x;
		sOriginY = y;
	}
	else
		wherexy(&sOriginX, &sOriginY);

	p = szTmpBuf;
	for (i = 0; ; ++i) {
			/* 输入一个字符 */
		ch = usr_getch();
			/* 回车就结束 */
		if (ch == VK_RETURN)
			break;
			/* 若是 delete 键或是 backspace 键 */
		if (VK_BACK == ch) {
				/* 若是回显的话, 则要根据窗口的大小及位置考虑输入之后光标的移动*/
			if (cStyle != NOECHO_INPUT_STYLE) {
					/* 首先获得当前光标的位置 */
				wherexy(&win_x, &win_y);
				if (win_y > sOriginY) {
					if (win_x != 1) {
							/* 往左擦出, 当然光标也要相应移动 */
						usr_putcharxy(win_x - 1, win_y, ' ');
						gotoxy(win_x - 1, win_y);
					}
					else {
							/* 如果光标在窗口的最左端, 那么在上一行的末尾输入一个字符, 并将光标跳到上一行的最末尾 */
						usr_putcharxy(g_curwin_width, win_y - 1, ' ');
						gotoxy(g_curwin_width, win_y - 1);
					}
				}
				else {
					if (win_x > sOriginX) {
							/* 往左擦出, 当然光标也要相应移动 */
						usr_putcharxy(win_x - 1, win_y, ' ');
						gotoxy(win_x - 1, win_y);
					}
					else {
							/* 光标已经运动到了输入开始处 */
						i = -1;
						p[0] = '\0';
						continue;
					}
				}
				p[i - 1] = '\0';
				i -= 2;
			}
			else {
					/* 若是输入不回显(光标位置不动) */
				if (i != 0) {
					p[i - 1] = '\0';
					i -= 2;
				}
				else {
					i = -1;
					p[0] = '\0';
				}
			}
			continue;
		}

		if (i >= iMaxInputChars - 1) {
			p[i] = '\0';
			bFull = TRUE;
		}
		else
			bFull = FALSE;

		if (bFull == FALSE)
			p[i] = ch;

		if (i > MAX_INPUT_LEN - 2 || bFull) {
			if (cStyle != NOECHO_INPUT_STYLE)
				putchar('\7');
			--i;
			continue;
		}

		if (cStyle == NORM_INPUT_STYLE) {
			if ('\t' == ch)	g_bshowtab = FALSE;
			usr_putchar(ch);
			if ('\t' == ch)	g_bshowtab = TRUE;
		}
		else if (cStyle == PASSWORD_INPUT_STYLE)
			usr_putchar('*');
	}

	if (i <= iMaxInputChars - 1)
		p[i] = '\0';

	memcpy(pStrBuf + 2, p, i + 1);
	pStrBuf[1] = i;
	if (wherey() < get_curwin_height())
		gotoxy(1, wherey()+1);
	else
		usr_cprintf("\r\n");
	return (pStrBuf + 2);
}


/*
 * usr_cgetline —— 从当前位置输入一串字符（最多maxlen个）
 */
char   *_Cdecl usr_cgetline(char buff[], int maxlen)
{
	buff[0] = maxlen;
	return usr_cgets(buff);
}


/*
 * gettext -- 保存指定的屏幕内容到指定的 buffer
 * @left:
 * @top: 区域左上角
 * @right:
 * @bottom: 区域右下角
 * @destin: 所存之处
 * 返回值: 成功1, 失败0
 */
int		_Cdecl gettext			(int left, int top, int right, int bottom,
								 void *destin)
{
	bool	bSuccess;
	COORD	coordBufSize;
	COORD	coordDest = {0, 0};
	SMALL_RECT	srReadRegion;

		/* 参数出错 */
	if (left <= 0 || left > get_conx() ||
		right <= 0 || right > get_conx() ||
		top <= 0 || top > get_cony() ||
		bottom <= 0 || bottom > get_cony() ||
		destin == NULL)
		return 0;

		/* 参数处理 */
	if (top > bottom)
		Swap(&top, &bottom, sizeof(int));
	if (left > right)
		Swap(&left, &right, sizeof(int));

		/* 设置要读取内容的矩形区域 */
	srReadRegion.Left	= (SHORT) (left - 1);
	srReadRegion.Top	= (SHORT) (top - 1);
	srReadRegion.Right	= (SHORT) (right - 1);
	srReadRegion.Bottom	= (SHORT) (bottom - 1);
	coordBufSize.X = (SHORT) (right - left + 1);
	coordBufSize.Y = (SHORT) (bottom - top + 1);
	bSuccess = ReadConsoleOutput(g_console, (PCHAR_INFO) destin, coordBufSize,
		coordDest, &srReadRegion);
	PERR(bSuccess, "ReadConsoleOutput");
	return 1;
}


/*
 * puttext -- 输出已保存的内容到指定的区域
 * @left:
 * @top: 区域左上角
 * @right:
 * @bottom: 区域右下角
 * @source: 来源之处
 * 返回值: 成功1, 失败0
 *
 *
 * 说明: 在 VC 下可能和 TC 有点不太一样, 计算的大小应该乘以 4. (例如: bufSize = 80 * 25 * 4)
 */
int		_Cdecl puttext			(int left, int top, int right, int bottom,
								 void *source)
{
	bool	bSuccess;
	COORD	coordBufSize;
	COORD	coordDest = {0, 0};
	SMALL_RECT	srReadRegion;

		/* 参数出错 */
	if (left <= 0 || left > get_conx() ||
		right <= 0 || right > get_conx() ||
		top <= 0 || top > get_cony() ||
		bottom <= 0 || bottom > get_cony() ||
		source == NULL)
		return 0;
		/* 参数处理*/
	if (top > bottom)
		Swap(&top, &bottom, sizeof(int));
	if (left > right)
		Swap(&left, &right, sizeof(int));
		/* 设置放置矩形区域 */
	srReadRegion.Left	= (SHORT) (left - 1);
	srReadRegion.Top	= (SHORT) (top - 1);
	srReadRegion.Right	= (SHORT) (right - 1);
	srReadRegion.Bottom	= (SHORT) (bottom - 1);
	coordBufSize.X = (SHORT) (right - left + 1);
	coordBufSize.Y = (SHORT) (bottom - top + 1);
	bSuccess = WriteConsoleOutput(g_console, (PCHAR_INFO) source, coordBufSize,
					coordDest, &srReadRegion);
	PERR(bSuccess, "WriteConsoleOutput");
	return 1;
}


/*
 * usr_getpass -- 和 getpass 类似功能, 先输出 prompt 内容, 然后要求输入, 最长 8 位
 * @prompt: 提示内容
 * 返回值: 输入的密码字符串
 */
char   *_Cdecl usr_getpass		(const char *prompt)
{
	static char password[11];
	int	iOldAttr;
	int	iCurCursorX, iCurCursorY;	

	iOldAttr = gettextattr();
	textattr(LIGHTGRAY + (BLACK << 4));
	usr_cprintf(prompt);

		/* 最长 8 个字符, 带上'\0' 共 9 个字符 */
	password[0] = 8 + 1;
	gets_handle(0, 0, FALSE, password, NOECHO_INPUT_STYLE);
		/* 貌似那个函数输入的时候字符都是黑底白字 */
	textattr(iOldAttr);

	wherexy(&iCurCursorX, &iCurCursorY);
	if (iCurCursorY != g_curwin_height)
		gotoxy(1, ++iCurCursorY);

	return (password + 2);
}


/*
 * usr_kbhit -- 检测键盘是否有键按下, 系统有这个函数, 直接调用得了
 */
int		_Cdecl usr_kbhit		(void)
{
	return _kbhit();
}


/*
 * usr_cscanf -- 输入格式串
 */
#if 0
int		_Cdecl usr_cscanf		(const char *format, ...)
{
	int			retval;
	va_list		ap;

	va_start(ap, format);
	retval = cscanf(format, ap);
	va_end(ap);
	return retval;
}
#endif


/*
 * clreoc -- clear end of column
 */
void	_Cdecl clreoc			(void)
{
	int		win_x, win_y,
			len, i;

		/* 获得当前光标的位置(窗口相关) */
	wherexy(&win_x, &win_y);
		/* 计算纵向要消除的字符数 */
	len = g_curwin_height - win_y + 1;
	for (i = 0; i < len; ++i)
		usr_putcharxy(win_x, win_y + i, ' ');
		/* 光标回到原先的位置 */
	gotoxy(win_x, win_y);
	return;
}

/*
 * inscolumn -- 从光标指定位置插入一列
 */
void	_Cdecl inscolumn		(void)
{
	int		sCurCursorX, sCurCursorY,
			scr_x, scr_y;

	wherexy(&sCurCursorX, &sCurCursorY);
		/* 假如是最右一列那就直接用空格去填充 */
	if (sCurCursorX == g_curwin_right) {
		gotoxy(sCurCursorX, 1);
		clreoc();
			/* 将光标移回原来的地方 */
		gotoxy(sCurCursorX, sCurCursorY);
	}
	else {
		pos_window2screen(sCurCursorX, sCurCursorY, &scr_x, &scr_y);
			/* 其他情况可以考虑用移动窗口来做 */
		movetext(scr_x, g_curwin_top, g_curwin_right, g_curwin_bottom,
			scr_x + 1, g_curwin_top);
	}
	return;
}


/*
 * delcolumn -- 从光标指定位置删除一列
 */
void	_Cdecl delcolumn		(void)
{
	int		sCurCursorX, sCurCursorY,
			scr_x, scr_y;

	wherexy(&sCurCursorX, &sCurCursorY);
		/* 假如是最右一行那就直接用空格去填充 */
	if (sCurCursorX == g_curwin_right) {
		gotoxy(sCurCursorX, 1);
		clreoc();
			/* 将光标移回原来的地方 */
		gotoxy(sCurCursorX, sCurCursorY);
	}
	else {
		pos_window2screen(sCurCursorX, sCurCursorY, &scr_x, &scr_y);
			/* 其他情况可以考虑用移动窗口来做 */
		movetext(scr_x + 1, g_curwin_top, g_curwin_right, g_curwin_bottom,
			scr_x, g_curwin_top);
	}
	return;
}


/*
 * get_cur_char_attr -- 获得当前字符的属性
 * 返回值: 获得的字符属性
 */
int		_Cdecl get_cur_char_attr(void)
{
	return get_certain_char_attr_handle(0, 0, FALSE);
}

/*
 * get_char_attrxy -- 获得指定字符的属性
 * @x:
 * @y: 指定位置
 * 返回值: 获得的字符属性
 */
int		_Cdecl get_char_attrxy	(int x, int y)
{
	if (x <= 0 || x > g_curwin_width ||
		y <= 0 || y > g_curwin_height)
		return 0;

	return get_certain_char_attr_handle(x, y, TRUE);
}

/*
 * get_certain_char_attr_handle -- 获得字符属性的主要实现函数
 * @x:
 * @y: 指定的位置
 * @bMoveCursor: 是否移动光标
 * 返回值: 获得的字符属性
 */
static int get_certain_char_attr_handle(int x, int y, bool bMoveCursor)
{
	bool	bSuccess;
	DWORD	dwCharsWritten;
	COORD	coordSource;
	int		char_attr;
	int		sCurCursorX, sCurCursorY,
			scr_x, scr_y;

		/* 参数检验 */
	if (bMoveCursor && (x <= 0 || x > g_curwin_width ||
		y <= 0 || y > g_curwin_height))
		return gettextattr();

		/* 移动光标并获得光标的当前位置 */
	if (bMoveCursor) {
		gotoxy(x, y);
		sCurCursorX = x;
		sCurCursorY = y;
	}
	else
		wherexy(&sCurCursorX, &sCurCursorY);
		/* 将窗口坐标转换为屏幕坐标 */
	pos_window2screen(sCurCursorX, sCurCursorY, &scr_x, &scr_y);
		/* 转换为实际物理坐标 */
	coordSource.X = (SHORT) (scr_x - 1);
	coordSource.Y = (SHORT) (scr_y - 1);
		/* 获取目标字符的属性 */
	bSuccess = ReadConsoleOutputAttribute(g_console, (LPWORD) &char_attr, sizeof(WORD),
		coordSource, &dwCharsWritten);
	PERR(bSuccess, "ReadConsoleOutputAttribute");
	return char_attr;
}


/*
 * backghighvideo -- 背景色高亮
 */
void	_Cdecl backghighvideo	(void)
{
	int		iCharBackg;

	iCharBackg = gettextbackground();
	iCharBackg <<= 4;
	iCharBackg |= (int) BACKGROUND_INTENSITY;
	iCharBackg >>= 4;

	b_backg_high_video = TRUE;
	textbackground(iCharBackg);
	return;
}


/*
 * showcursor -- 显示光标
 */
void	_Cdecl showcursor		(void)
{
	set_cursor_mode_handle(-1, TRUE);
}


/*
 * closecursor -- 关闭光标
 */
void	_Cdecl closecursor		(void)
{
	set_cursor_mode_handle(-1, FALSE);
}


/*
 * setcursorsize -- 设置光标的大小 (1 ~ 100)
 * @size: 要设置的光标的大小
 */
void	_Cdecl setcursorsize	(int size)
{
	if (size <=0 || size > 100)
		return;

	set_cursor_mode_handle(size, TRUE);
}

/*
 * set_cursor_mode_handle -- 设置光标属性的主要实现函数
 * @size: 要设置的光标大小
 * @bVisiable: 光标是否可见
 */
static void set_cursor_mode_handle(int size, bool bVisiable)
{
	CONSOLE_CURSOR_INFO	cci;
	bool	bSuccess;
	bool	bModifySize = FALSE,
			bModifyShowOffState = FALSE;;

	bSuccess = GetConsoleCursorInfo(g_console, &cci);
	PERR(bSuccess, "GetConsoleCursorInfo");

	if (size > 0 && size <= 100) {
		cci.dwSize = (DWORD) size;
		bModifySize = TRUE;
	}

	if (!bModifySize) {
		if (cci.bVisible == bVisiable)
			return;

		cci.bVisible = bVisiable;
	}

	bSuccess = SetConsoleCursorInfo(g_console, &cci);
	PERR(bSuccess, "SetConsoleCursorInfo");
	return;
}


/*
 * input_passwd -- 输入密码(回显 '*')
 * 返回值: 输入的密码字符串
 * 说明: strbuf[0]为最大输入个数, strbuf[1] 返回实际输入的字符个数
 */
char   *_Cdecl input_passwd		(char *strbuf)
{
	if (NULL == strbuf)
		return NULL;

	return gets_handle(0, 0, FALSE, strbuf, PASSWORD_INPUT_STYLE);
}


/*
 * 说明: 以下几个函数和上面这个类似...
 */
char   *_Cdecl input_passwdxy	(int x, int y, char *strbuf)
{
	if (x <= 0 || x > g_curwin_width ||
		y <= 0 || y > g_curwin_height ||
		NULL == strbuf)
		return NULL;

	return gets_handle(x, y, TRUE, strbuf, PASSWORD_INPUT_STYLE);
}

char   *_Cdecl input_noecho		(char *strbuf)
{
	if (NULL == strbuf)
		return NULL;

	return gets_handle(0, 0, FALSE, strbuf, NOECHO_INPUT_STYLE);
}

char   *_Cdecl input_noechoxy	(int x, int y, char *strbuf)
{
	if (x <= 0 || x > g_curwin_width ||
		y <= 0 || y > g_curwin_height ||
		NULL == strbuf)
		return NULL;

	return gets_handle(x, y, TRUE, strbuf, NOECHO_INPUT_STYLE);
}

char   *_Cdecl usr_cgetsxy		(int x, int y, char *strbuf)
{
	if (x <= 0 || x > g_curwin_width ||
		y <= 0 || y > g_curwin_height ||
		NULL == strbuf)
		return NULL;

	return gets_handle(x, y, TRUE, strbuf, NORM_INPUT_STYLE);
}


/*
 * set_char_special_mode -- 设置当前所有字符的特殊属性
 *
 * 说明: 只有在中文显示页下才有效
 */
void set_char_special_mode(enum special_char_attr char_mode)
{
	int				wTmpCharAttr;
	unsigned int	wCodeCP;

	wCodeCP = get_codepage();
	if (wCodeCP != CHINESE_OUTPUT_CP)
		return;

	wTmpCharAttr = gettextattr();
	wTmpCharAttr = (wTmpCharAttr & (int) 0x000000FF) | char_mode;
	textattr(wTmpCharAttr);
	return;
}


/*
 * reset_char_special_mode -- 恢复当前所有字符的特殊属性为无
 */
void reset_char_special_mode(void)
{
	unsigned int	wCodeCP;

	wCodeCP = get_codepage();
	if (wCodeCP != CHINESE_OUTPUT_CP)
		return;

	set_char_special_mode(RESET_NORM_MODE);
}


/*
 * get_char_special_mode -- 获得当前所有字符的特殊属性
 */
int  get_char_special_mode(void)
{
	int				wTmpCharMode;
	unsigned int	wCodeCP;

	wCodeCP = get_codepage();
	if (wCodeCP != CHINESE_OUTPUT_CP)
		return RESET_NORM_MODE;

	wTmpCharMode = gettextattr();
	return (wTmpCharMode & (int) 0x0000FF00);
}


/*
 * box -- 画一个方框
 * @left:
 * @top: 方框左上角
 * @right:
 * @bottom: 方框右下角 (窗口坐标)
 * @char_attr: 边框字符的属性
 * @single: 是否单框, 否则双线框
 */
void box(int left, int top, int right, int bottom, int char_attr, bool single)
{
	bool	bSuccess;
	COORD	coordStart, coordEnd;
	COORD	coordBuf;
	int		dwFillSize, dwCharsWritten;
	int		wFrameType;
	int		iOldCharAttr, iTmpOutputCharAttr;
	int		i;
	int		scr_x, scr_y,
			sOldWinX, sOldWinY;
	int		cBoxCar[2][6] = {{218, 196, 191, 179, 192, 217},		/* 单边框ASCII数组 */
							{0xc9, 0xcd, 0xbb, 0xba, 0xc8, 0xbc}};	/* 双边框ASCII数组 */

		/* 参数出错, 不填充 */
	if (left <= 0 || left > g_curwin_width ||
		right <= 0 || right > g_curwin_width ||
		top <= 0 || top > g_curwin_height ||
		bottom <= 0 || bottom > g_curwin_height)
		return;

		/* 参数处理*/
	if (top > bottom)
		Swap(&top, &bottom, sizeof(int));
	if (left > right)
		Swap(&left, &right, sizeof(int));

		/* 保存一开始的坐标值 */
	wherexy(&sOldWinX, &sOldWinY);
		/* 转化左上角坐标到屏幕坐标 */
	pos_window2screen(left, top, &scr_x, &scr_y);
	coordStart.X = (SHORT) (scr_x - 1);
	coordStart.Y = (SHORT) (scr_y - 1);
		/* 转化右下角坐标到屏幕坐标 */
	pos_window2screen(right, bottom, &scr_x, &scr_y);
	coordEnd.X = (SHORT) (scr_x - 1);
	coordEnd.Y = (SHORT) (scr_y - 1);
		/* 太小了, 不够话边框 */
	if (coordEnd.X - coordStart.X < 2 ||
		coordEnd.Y - coordStart.Y < 2)
		return;

		/* 保存当前的颜色设置 */
	iOldCharAttr = gettextattr();

	if (g_cur_screen_mode == BW40 || g_cur_screen_mode == BW80)
		iTmpOutputCharAttr = (char_attr & (int) 0x0000FF88) | (int) 0x00000007;
	else if (g_cur_screen_mode == MONO)
		iTmpOutputCharAttr = iOldCharAttr & (int) 0x0000FF8F;
	else
		iTmpOutputCharAttr = char_attr;

		/* 设置成规定的边框颜色 */
	textattr(iTmpOutputCharAttr);
		/* 设置边框类型, 双还是单 */
	wFrameType = single ? 0 : 1;
		/* 下面就是具体的画边框函数 */

		/* 左上角 */
	usr_putcharxy(left, top, cBoxCar[wFrameType][0]);	
		/* 右上角 */
	usr_putcharxy(right, top, cBoxCar[wFrameType][2]);	
		/* 左下角 */
	usr_putcharxy(left, bottom, cBoxCar[wFrameType][4]);
		/* 右下角 */
	usr_putcharxy(right, bottom, cBoxCar[wFrameType][5]);	
		/* 上端水平线 */
	coordBuf = coordStart;
	++coordBuf.X;
	dwFillSize = (coordEnd.X - coordStart.X - 1);
	bSuccess = FillConsoleOutputCharacter(g_console, (TCHAR) cBoxCar[wFrameType][1], dwFillSize,
			coordBuf, &dwCharsWritten);
	PERR(bSuccess, "FillConsoleOutputCharacter");
	bSuccess = FillConsoleOutputAttribute(g_console, (WORD) iTmpOutputCharAttr, dwFillSize,
			coordBuf, &dwCharsWritten);
	PERR(bSuccess, "FillConsoleOutputAttribute");
		/* 下端水平线 */
	coordBuf.Y = coordEnd.Y;
	bSuccess = FillConsoleOutputCharacter(g_console, (TCHAR) cBoxCar[wFrameType][1], dwFillSize,
			coordBuf, &dwCharsWritten);
	PERR(bSuccess, "FillConsoleOutputCharacter");
	bSuccess = FillConsoleOutputAttribute(g_console, (WORD) iTmpOutputCharAttr, dwFillSize,
			coordBuf, &dwCharsWritten);
	PERR(bSuccess, "FillConsoleOutputAttribute");
		/*输出两端竖线*/
	for (i = 0; i < bottom - top - 1; ++i) {
		usr_putcharxy(left, top + 1 + i, cBoxCar[wFrameType][3]);
		usr_putcharxy(right, top + 1 + i, cBoxCar[wFrameType][3]);
	}
		/* 恢复原先的颜色设置 */
	textattr(iOldCharAttr);
		/* 回到左上角 */
	gotoxy(left, top);
	return;
}


/*
 * draw3Dwindow -- 开辟一个有 3D 效果的活动窗口
 * @left:
 * @top: 窗口左上角
 * @right:
 * @bottom: 窗口右下角 (屏幕坐标)
 * @table_attr: 开辟的窗口的颜色设置
 */
void draw3Dwindow(int left, int top, int right, int bottom, int table_attr)
{
	int		iOldCharAttr, iTmpOutputCharAttr;

		/* 检验参数 */
	if (left <= 0 || left > get_conx() ||
		right <= 0 || right > get_conx() ||
		top <= 0 || top > get_cony() ||
		bottom <= 0 || bottom > get_cony())
		return;

	iOldCharAttr = gettextattr();

	if (g_cur_screen_mode == BW40 || g_cur_screen_mode == BW80)
		iTmpOutputCharAttr = (table_attr & (int) 0x0000FF88) | (int) 0x00000007;
	else if (g_cur_screen_mode == MONO)
		iTmpOutputCharAttr = iOldCharAttr & (int) 0x0000FF8F;
	else
		iTmpOutputCharAttr = table_attr;

		/* 参数处理*/
	if (top > bottom)
		Swap(&top, &bottom, sizeof(int));
	if (left > right)
		Swap(&left, &right, sizeof(int));

	fill_rect_char(left + 1, top + 1, right + 1, bottom + 1, BLACK, ' ');
		/* 设置当前的活动窗口 */
	window(left, top, right, bottom);
	textattr(table_attr);
	clrscr();
	return;
}


/*
 * draw_horizen_line -- 画水平线
 * @x1:
 * @y1: 起始坐标
 * @x2: 终止横坐标
 * @char_attr: 线条颜色设置(不改变当前整个窗口的颜色设置)
 * @ch: 所画的字符
 */
void draw_horizen_line(int x1, int y1, int x2, int char_attr, int ch)
{
	char	szTmp[128];
	int		len;
	bool	bSuccess;
	int		dwCharsWritten;
	COORD	coordFrom;
	int		scr_x, scr_y;
	int		iOldCharAttr, iTmpOutputCharAttr;

		/* 参数检验 */
	if (x1 <= 0 || x1 > g_curwin_width ||
		x2 <= 0 || x2 > g_curwin_width ||
		y1 <= 0 || y1 > g_curwin_height)
		return;

	iOldCharAttr = gettextattr();

	if (g_cur_screen_mode == BW40 || g_cur_screen_mode == BW80)
		iTmpOutputCharAttr = (char_attr & (int) 0x0000FF88) | (int) 0x00000007;
	else if (g_cur_screen_mode == MONO)
		iTmpOutputCharAttr = iOldCharAttr & (int) 0x0000FF8F;
	else
		iTmpOutputCharAttr = char_attr;

		/* 参数处理 */
	if (x1 > x2)
		Swap(&x1, &x2, sizeof(int));

	pos_window2screen(x1, y1, &scr_x,&scr_y);
	coordFrom.X = (SHORT) (scr_x - 1);
	coordFrom.Y = (SHORT) (scr_y - 1);
	len = x2 - x1 + 1;
	memset(szTmp, ch, len);
	bSuccess = WriteConsoleOutputCharacter(g_console, szTmp, len,
		coordFrom, &dwCharsWritten);
	PERR(bSuccess, "WriteConsoleOutputCharacter");
	bSuccess = FillConsoleOutputAttribute(g_console, (WORD) iTmpOutputCharAttr, len,
		coordFrom, &dwCharsWritten);
	PERR(bSuccess, "FillConsoleOutputAttribute");
	return;
}


/*
 * draw_vertical_line -- 画铅垂线
 * @x1:
 * @y1: 起始坐标
 * @y2: 终止纵坐标
 * @char_attr: 线条颜色设置(不改变当前整个窗口的颜色设置)
 * @ch: 所画的字符
 */
void draw_vertical_line(int x1, int y1, int y2, int char_attr, int ch)
{
	int		len, i,
			iOldCharAttr, iTmpOutputCharAttr;

	if (x1 <= 0 || x1 > g_curwin_width ||
		y1 <= 0 || y1 > g_curwin_width ||
		y2 <= 0 || y2 > g_curwin_height)
		return;

	iOldCharAttr = gettextattr();

	if (g_cur_screen_mode == BW40 || g_cur_screen_mode == BW80)
		iTmpOutputCharAttr = (char_attr & (int) 0x0000FF88) | (int) 0x00000007;
	else if (g_cur_screen_mode == MONO)
		iTmpOutputCharAttr = iOldCharAttr & (int) 0x0000FF8F;
	else
		iTmpOutputCharAttr = char_attr;

	if (y1 > y2)
		Swap(&y1, &y2, sizeof(int));

		/* 设置成规定颜色 */
	textattr(iTmpOutputCharAttr);
		/* 具体画竖线 */
	len = y2 - y1 + 1;
	for (i = 0; i < len; ++i)
		usr_putcharxy(x1, y1 + i, ch);
		/* 设置回原来的颜色 */
	textattr(iOldCharAttr);
	return;
}


/*
 * comfirm_box -- 弹出一个确认框, 按 ESC 或 ENTER 返回
 * @left:
 * @top: 确认框左上角
 * @note_msg: 输出信息字符串
 * @btype: 确认框类型, 有普通输出信息型, 警告型, 错误型
 */
void comfirm_box(int left, int top, const char *note_msg, enum box_type btype)
{
	char	*pcScreenBuf = NULL;
	int		iOldCharAttr, iTmpOutputCharAttr,
			len = 0, x, y,
			key;
	struct text_info r;

		/* 检验参数 */
	if (left <= 0 || left > get_conx() ||
		top <= 0 || top > get_cony() ||
		NULL == note_msg)
		return;

	wherexy(&x, &y);
	closecursor();

	gettextinfo(&r);
	iOldCharAttr = r.attribute;

	if (NORM_TYPE == btype)
		MessageBeep(MB_ICONASTERISK);
	else if (WARN_TYPE == btype)
		MessageBeep(MB_ICONEXCLAMATION);
	else
		MessageBeep(MB_ICONHAND);

	if (g_cur_screen_mode == BW40 || g_cur_screen_mode == BW80)
		iTmpOutputCharAttr = WHITE + (BLACK << 4);
	else if (g_cur_screen_mode == MONO)
		iTmpOutputCharAttr = iOldCharAttr & (int) 0x0000FF8F;
	else if (NORM_TYPE == btype)
		iTmpOutputCharAttr = YELLOW + (BLUE << 4);
	else if (WARN_TYPE == btype)
		iTmpOutputCharAttr = LIGHTCYAN + (BLUE << 4);
	else
		iTmpOutputCharAttr = WHITE + (RED << 4);

	window(1, 1, get_conx(), get_cony());
	textattr(iTmpOutputCharAttr);

	len = lstrlen(note_msg);

	pcScreenBuf = (char *) malloc((len + 4 + 1) * (10 + 1) * 4);
	PERR(pcScreenBuf, "malloc");

	gettext(left, top, left + len + 4, top + 10, pcScreenBuf);
	fill_rect_char(left, top, left + len + 4, top + 10, iTmpOutputCharAttr, ' ');
	box(left, top, left + len + 4, top + 10, iTmpOutputCharAttr, TRUE);
	usr_putcharxy(left + 2, top, '|');
	if (NORM_TYPE == btype)
		usr_cprintf(" NOTE |");
	else if (WARN_TYPE == btype)
		usr_cprintf(" WARN |");
	else
		usr_cprintf(" ERROR |");

	window(left + 2, top + 3, left + len + 2, top + 4);
	usr_cputs(note_msg);

	window(1, 1, get_conx(), get_cony());
	box(left + len/2 - 3, top + 6, left + len/2 + 5, top + 8, iTmpOutputCharAttr, TRUE);
	usr_cprintfcolorxy(left + len/2 - 2, top + 7, WHITE + (BLACK << 4), "  O K  ");

	for ( ; ; ) {
		while (bioskey(1) == 0)
			;
		key = bioskey(0);
		if (KEY_ESC == key || KEY_ENTER == key)
			break;
		MessageBeep(MB_OK);
	}

	puttext(left, top, left + len + 4, top + 10, pcScreenBuf);
	free(pcScreenBuf);

	window(r.winleft, r.wintop, r.winright, r.winbottom);
	textattr(iOldCharAttr);
	showcursor();
	gotoxy(x, y);
	return;
}


/*
 * get_curwin_width —— 获得当前窗口的宽度
 */
int get_curwin_width(void)
{
	return g_curwin_width;
}


/*
 * get_curwin_height —— 获得当前窗口的高度
 */
int get_curwin_height(void)
{
	return g_curwin_height;
}


/*
 * get_curwin —— 获得当前窗口的屏幕坐标值
 */
void get_curwin(int *left, int *top, int *right, int *bottom)
{
	if (left	!= NULL)	*left	= g_curwin_left;
	if (top		!= NULL)	*top	= g_curwin_top;
	if (right	!= NULL)	*right	= g_curwin_right;
	if (bottom	!= NULL)	*bottom	= g_curwin_bottom;
}


/*
 * skip_blanks —— 定位str字符串中的第一个非空字符
 */
char *skip_blanks(const char *str)
{
	if (NULL == str)
		return NULL;
	while (' ' == *str || '\t' == *str)
		++str;
	return (char *) str;
}
