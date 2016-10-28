#include <Winsock2.h>
#include "myconio.h"	/* stdio.h is also included */
#include "dxyh.h"
#include "error.h"
#include "record.h"
#include "login.h"

#pragma comment(lib, "WS2_32.lib")

int main(int argc, char **argv)
{
	SOCKET		sock_clit;

	initscr();
	if (FTP_OK == login()) {
		config();
		sock_clit = ftp_connect2serv();
		ftp_do_loop(sock_clit);
	}
	atlast();
	endwin();
	return 0;
}
