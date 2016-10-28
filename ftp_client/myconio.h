#ifndef _MYCONIO_H
#define _MYCONIO_H

#if     !defined(_WIN32) && !defined(_MAC)
#error ERROR: Only Mac or Win32 targets supported!
#endif

#if _STDC__
#define _Cdecl
#else
#define _Cdecl cdecl
#endif

#ifndef __OLDCONIO__

struct text_info {
	unsigned char winleft;
	unsigned char wintop;
	unsigned char winright;
	unsigned char winbottom;
	unsigned char attribute;
	unsigned char normattr;
	unsigned char currmode;
	unsigned char screenheight;
	unsigned char screenwidth;
	unsigned char curx;
	unsigned char cury;
};

enum text_modes { LASTMODE=-1, BW40=0, C40, BW80, C80, MONO=7 };

#if	!defined(__COLORS)
#define __COLORS

enum COLORS {
	BLACK,
	BLUE,
	GREEN,
	CYAN,
	RED,
	MAGENTA,
	BROWN,
	LIGHTGRAY,
	DARKGRAY,
	LIGHTBLUE,
	LIGHTGREEN,
	LIGHTCYAN,
	LIGHTRED,
	LIGHTMAGENTA,
	YELLOW,
	WHITE
};
#endif

#include <stdio.h>
#include <conio.h>

//#define BLINK		128

void	_Cdecl clreol			(void);
void	_Cdecl clrscr			(void);
void	_Cdecl delline			(void);
int		_Cdecl gettext			(int left, int top, int right, int bottom,
								 void *destin);
void	_Cdecl gettextinfo		(struct text_info *r);
void 	_Cdecl gotoxy			(int x, int y);
void	_Cdecl highvideo		(void);
void 	_Cdecl insline			(void);
void	_Cdecl lowvideo			(void);
int		_Cdecl movetext			(int left, int top, int right, int bottom,
								 int destleft, int desttop);
void	_Cdecl normvideo		(void);
int		_Cdecl puttext			(int left, int top, int right, int bottom,
								 void *source);
void	_Cdecl textattr			(int newattr);
void 	_Cdecl textbackground	(int newcolor);
void 	_Cdecl textcolor		(int newcolor);
void 	_Cdecl textmode			(int newmode);
int  	_Cdecl wherex			(void);
int  	_Cdecl wherey			(void);
void 	_Cdecl window			(int left, int top, int right, int bottom);
#endif

char   *_Cdecl usr_cgets		(char *str);
int		_Cdecl usr_cprintf		(const char *format, ...);
int		_Cdecl usr_cputs		(const char *str);
#define usr_cscanf	scanf
// int		_Cdecl usr_cscanf		(const char *format, ...);
int		_Cdecl usr_getch		(void);
int		_Cdecl usr_getche		(void);
char   *_Cdecl usr_getpass		(const char *prompt);
int		_Cdecl usr_kbhit		(void);
int		_Cdecl usr_putch		(int c);
int		_Cdecl usr_ungetch		(int ch);
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

void	_Cdecl initscr			(void);										/* 这个函数在程序开始时调用, 初始化一些量 */
void	_Cdecl endwin			(void);										/* 这个函数在程序结束时调用 */
int		_Cdecl usr_putchar		(int c);									/* 输出字符, 和 usr_putch 功能一样 */
int		_Cdecl usr_putcharxy	(int x, int y, int c);						/* 在窗口中指定坐标处输出字符 */
int		_Cdecl usr_cprintfxy	(int x, int y, const char *format, ...);	/* 在窗口中指定坐标处输出格式串 */
int		_Cdecl usr_cprintfcolor	(int attr, const char *format, ...);		/* 在窗口中当前位置处输出指定颜色属性的格式串 */
int		_Cdecl usr_cprintfcolorxy(int x, int y, int attr, const char *fmt, ...);/* 在窗口指定坐标处输出指定颜色属性格式串 */
void	_Cdecl wherexy			(int *x, int *y);							/* 获得当前光标的窗口坐标值 */
void	_Cdecl where_scr_xy		(int *scr_x, int *scr_y);					/* 获得当前光标的屏幕坐标值 */
int		_Cdecl where_scr_x		(void);										/* 获得当前光标的屏幕水平坐标值 */
int		_Cdecl where_scr_y		(void);										/* 获得当前光标的屏幕垂直坐标值 */
int		_Cdecl get_curwin_width	(void);										/* 获得当前窗口的宽度 */
int		_Cdecl get_curwin_height(void);										/* 获得当前窗口的高度 */
void	_Cdecl get_curwin		(int *l, int *t, int *r, int *b);			/* 获得当前窗口的屏幕坐标值 */
int		_Cdecl gettextcolor		(void);										/* 获得当前的前景色 */
int		_Cdecl gettextbackground(void);										/* 获得当前的背景色 */
int		_Cdecl gettextattr		(void);										/* 获得当前的颜色设置(前/背景色), 若中文显示页, 也包括字符的特殊属性 */
int		_Cdecl bioskey			(int cmd);									/* 和 TC 下的 bioskey 相对, 功能类似 */
void	_Cdecl clreoc			(void);										/* 和 clreol 相对, 删除当前位置到窗口最下端的那一列 */
void	_Cdecl inscolumn		(void);										/* 和 insline 相对, 增加一列 */
void	_Cdecl delcolumn		(void);										/* 和 delline 相对, 删除一列 */
void	_Cdecl backghighvideo	(void);										/* 背景色高亮 */
void	_Cdecl showcursor		(void);										/* 显示光标 */
void	_Cdecl closecursor		(void);										/* 关闭光标 */
void	_Cdecl setcursorsize	(int size);									/* 设置光标大小(0~100) */
int		_Cdecl get_cur_char_attr(void);										/* 获得当前光标所在字符的属性 */
int		_Cdecl get_char_attrxy	(int x, int y);								/* 获得指定位置字符的属性 (窗口坐标) */
char   *_Cdecl usr_cgetsxy		(int x, int y, char *strbuf);				/* 指定位置开始输入字符串, strbuf[0]为输入最大字符数 */
char   *_Cdecl usr_cgetline		(char buff[], int maxlen);					/* 当前位置输入字符串，最多maxlen个字符 */
char   *_Cdecl input_passwd		(char *strbuf);								/* 当前位置开始输入密码(回显 '*') */
char   *_Cdecl input_passwdxy	(int x, int y, char *strbuf);				/* 指定位置开始输入密码(回显 '*') */
char   *_Cdecl input_noecho		(char *strbuf);								/* 当前位置开始输入(不回显) */
char   *_Cdecl input_noechoxy	(int x, int y, char *strbuf);				/* 指定位置开始输入(不回显) */
char   *_Cdecl skip_blanks		(const char *str);							/* 掠过str字符串中的前导空格、制表符 */

	/* 按键对应的键值 */
#define KEY_LEFT		0X4B00
#define KEY_UP			0X4800
#define KEY_RIGHT		0X4D00
#define KEY_DOWN		0X5000
#define KEY_ESC			0X1B
#define KEY_F1			0X3B00
#define KEY_F2			0X3C00
#define KEY_F3			0X3D00
#define KEY_F4			0X3E00
#define KEY_F5			0X3F00
#define KEY_F6			0X4000
#define KEY_F7			0X4100
#define KEY_F8			0X4200
#define KEY_F9			0X4300
#define KEY_F10			0X4400
#define KEY_F11			0X8500
#define KEY_F12			0X8600
#define KEY_ENTER		0X0D
#define KEY_HOME		0X4700
#define KEY_END			0X4F00
#define KEY_BACKSPACE	0X08
#define KEY_PAGEUP		0X4900
#define KEY_PAGEDOWN	0X5100
#define KEY_INSERT		0X5200
#define KEY_DELETE		0X5300

#define KEY_ALT_A		0X1E00
#define KEY_ALT_B		0X3000
#define KEY_ALT_C		0X2E00
#define KEY_ALT_D		0X2000
#define KEY_ALT_E		0X1200
#define KEY_ALT_F		0X2100
#define KEY_ALT_G		0X2200
#define KEY_ALT_H		0X2300
#define KEY_ALT_I		0X1700
#define KEY_ALT_M		0X3200
#define KEY_ALT_N		0X3100
#define KEY_ALT_O		0X1800
#define KEY_ALT_P		0X1900
#define KEY_ALT_Q		0X1000
#define KEY_ALT_S		0X1F00
#define KEY_ALT_T		0X1400
#define KEY_ALT_V		0X2F00
#define KEY_ALT_X		0X2D00
#define KEY_ALT_F1		0X6800
#define KEY_ALT_F2		0X6900
#define KEY_ALT_F3		0X6A00
#define KEY_ALT_F4		0X6B00
#define KEY_ALT_F5		0X6C00
#define KEY_ALT_F6		0X6D00
#define KEY_ALT_F7		0X6E00
#define KEY_ALT_F8		0X6F00

#define KEY_CTRL_A		0X1
#define KEY_CTRL_B		0X2
#define KEY_CTRL_C		0X3
#define KEY_CTRL_D		0X4
#define KEY_CTRL_E		0X5
#define KEY_CTRL_F		0X6
#define KEY_CTRL_G		0X7
#define KEY_CTRL_H		0X8
#define KEY_CTRL_I		0X9
#define KEY_CTRL_J		0XA
#define KEY_CTRL_K		0XB
#define KEY_CTRL_L		0XC
#define KEY_CTRL_M		0XD
#define KEY_CTRL_N		0XE
#define KEY_CTRL_O		0XF
#define KEY_CTRL_P		0X10
#define KEY_CTRL_Q		0X11
#define KEY_CTRL_R		0X12
#define KEY_CTRL_S		0X13
#define KEY_CTRL_T		0X14
#define KEY_CTRL_U		0X15
#define KEY_CTRL_V		0X16
#define KEY_CTRL_W		0X17
#define KEY_CTRL_X		0X18
#define KEY_CTRL_Y		0X19
#define KEY_CTRL_Z		0X1A

typedef char			bool;
#define FALSE			0
#define TRUE			1
#define MAX_CHAR_LINE	127
#define TABSIZE			8
#define BUFSIZE			4096
extern int tabstop;

#define MS_DOS_OUTPUT_CP		437		/* 可显示扩展Ascii码, 但不能显示中文! */
#define CHINESE_OUTPUT_CP		936		/* 可显示中文, 但不能显示扩展Ascii码! */
enum special_char_attr {
	GRID_HORIZONTAL	= 0x0400,			/* 每个字符上面有一个小短横 */
	GRID_LVERTICAL	= 0x0800,			/* 每个字符左面有一个小竖横 */
	GRID_RVERTICAL	= 0x1000,			/* 每个字符右面有一个小竖横 */
	REVERSE_VIDEO	= 0x4000,			/* 颜色反转 */
	UNDERSCORE		= 0x8000,			/* 每个字符下面有一个小短横(下划线) */
	RESET_NORM_MODE	= 0x0000			/* 取消特殊属性 */
};

#define PERR(bSuccess, api) {if (!(bSuccess)) perr(__FILE__, __LINE__, \
	api, GetLastError());}
void perr(const char *szFileName, int line, const char *szApiName, int err);/* 错误处理函数 */
void resize_con_buf_and_window(int xsize, int ysize);						/* 调整窗口及窗口 buffer 的大小 */
void set_codepage(unsigned int codepageID);									/* 设置显示页 */
unsigned int get_codepage(void);											/* 设置控制台的标题 */
void set_con_title(const char *title);										/* 设置字符的特殊属性等等 */
void set_char_special_mode(enum special_char_attr char_mode);
void reset_char_special_mode(void);
int  get_char_special_mode(void);

void pos_screen2window(int scr_x, int scr_y, int *win_x, int *win_y);		/* 辅助功能函数, 将屏幕坐标转化为窗口坐标 */
void pos_window2screen(int win_x, int win_y, int *scr_x, int *scr_y);		/* 辅助功能函数, 将窗口坐标转化为屏幕坐标 */
void Swap(void *item1, void *item2, size_t size);							/* 辅助功能函数, 任意类型变量交换 */
int get_conx(void);															/* 获得当前控制台窗口的横向长度 */
int get_cony(void);															/* 获得当前控制台窗口的纵向长度 */

void fill_rect_attr(int left, int top, int right, int bottom, int char_attr);	/* 改变指定区域字符属性(不改变当前的颜色设置) */
void fill_rect_char(int left, int top, int right, int bottom, int char_attr, int fill_char);/* 以特定属性字符填充(同上) */

void box(int left, int top, int right, int bottom, int char_attr, bool single);	/* 画边框 */
void draw3Dwindow(int left, int top, int right, int bottom, int table_attr);	/* 开辟一个有 3D 效果的活动窗口 */

void draw_horizen_line(int x1, int y1, int x2, int char_attr, int ch);		/* 画水平线 */
void draw_vertical_line(int x1, int y1, int y2, int char_attr, int ch);		/* 画垂直线 */

void set_Nth_bit(void *val, size_t size, int index);						/* 第 n 位置位 */
void reset_Nth_bit(void *val, size_t size, int index);						/* 第 n 位复位 */
char get_low_byte(void *val, size_t size);									/* 获得低字节 */
char get_high_byte(void *val, size_t size);									/* 获得高字节 */
bool test_Nth_bit(void *val, size_t size, int index);						/* 测试第 n 位 */
void flush_inputbuf(void);													/* 清空输入缓冲区 */

enum box_type {	/* 确认框的类型, 共 3 种 */
	NORM_TYPE,	/* 蓝底黄字 */
	WARN_TYPE,	/* 蓝底红字 */
	ERROR_TYPE	/* 红底白字 */
};
void comfirm_box(int left, int top, const char *note_msg, enum box_type btype);
#endif
