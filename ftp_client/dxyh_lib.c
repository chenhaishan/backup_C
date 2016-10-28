#include <Winsock2.h>
#include <stdio.h>
#include <io.h>
#include "dxyh.h"
#include "myconio.h"
#include "error.h"

SOCKET Socket(int af, int type, int protocol)
{
	SOCKET	sock;

	if (INVALID_SOCKET == (sock = socket(af, type, protocol))) {
		WSACleanup();
		sock_err_sys_q("socket error");
	}
	return sock;
} /* end Socket */

void Bind(SOCKET s, const struct sockaddr FAR *name, int namelen)
{
	if (SOCKET_ERROR == bind(s, name, namelen)) {
		WSACleanup();
		sock_err_sys_q("bind error");
	}
} /* end Bind */

void Listen(SOCKET s, int backlog)
{
	if (SOCKET_ERROR == listen(s, backlog)) {
		WSACleanup();
		sock_err_sys_q("listen error");
	}
} /* end Listen */

SOCKET Accept(SOCKET s, struct sockaddr FAR *addr, int FAR *addrlen)
{
	SOCKET	sock;

	if (INVALID_SOCKET == (sock = accept(s, addr, addrlen))) {
		WSACleanup();
		sock_err_sys_q("accept error");
	}
	return sock;
} /* end Accept */

int _Write(int fd, const void *buffer, unsigned int count)
{
	ssize_t		n;

	if (-1 == (n = _write(fd, buffer, count)))
		std_err_sys_q("_write error");
	return n;
} /* end _Write */

int _Read(int fd, void *buffer, unsigned int count)
{
	ssize_t		n;

	if (-1 == (n = _read(fd, buffer, count)))
		std_err_sys_q("_read error");
	return n;
} /* end _Read */

int writen(int fd, const void *buffer, unsigned int n)
{
	size_t		bytestowrite;
	ssize_t		byteswritten;
	const char	*ptr = NULL;

	for (ptr = buffer, bytestowrite = n;
		bytestowrite > 0;
		ptr += byteswritten, bytestowrite -= byteswritten) {
		byteswritten = write(fd, ptr, bytestowrite);
		if (-1 == byteswritten && errno != EINTR)
			return -1;
		if (-1 == byteswritten)
			byteswritten = 0;
	}
	return n;
} /* end writen */

int Writen(int fd, const void *buffer, unsigned int n)
{
	ssize_t		len;

	if (-1 == (len = writen(fd, buffer, n)))
		std_err_sys_q("Writen error");
	return len;
} /* end Writen */

int readn(int fd, void *buffer, unsigned int n)
{
	size_t		bytestoread;
	ssize_t		bytesread;
	char		*ptr = NULL;

	for (ptr = buffer, bytestoread = n;
		 bytestoread > 0;
		 ptr += bytesread, bytestoread -= bytesread) {
		if ((bytesread = read(fd, ptr, bytestoread)) < 0) {
			if (EINTR == errno)
				bytesread = 0;
			else
				return -1;
		} else if (0 == bytesread)
			break;
	}
	return (n - bytestoread);
} /* end readn */

int Readn(int fd, void *buffer, unsigned int n)
{
	ssize_t		len;

	if (-1 == (len = readn(fd, buffer, n)))
		std_err_sys_q("Readn error");
	return len;
} /* end Readn */

int Read1(int fd, char *ptr)
{
	static int read_cnt = 0;
	static char *read_ptr = NULL;
	static char read_buf[MAXLINE];

	if (read_cnt <= 0) {
		again:
		if (-1 == (read_cnt = _read(fd, read_buf, sizeof(read_buf)))) {
			if (EINTR == errno)
				goto again;
			return -1;
		}
		else if (0 == read_cnt)
			return 0;
		read_ptr = read_buf;
	}
	--read_cnt;
	*ptr = *read_ptr++;
	return 1;
}

int Readline(int fd, void *vptr, unsigned int maxlen)
{
	ssize_t		n, rc;
	char		c, *ptr = vptr;

	for (n = 1; (unsigned int) n < maxlen; ++n) {
again:
		if (1 == (rc = Read1(fd, &c))) {
			*ptr++ = c;
			if ('\n' == c)
				break;
		}
		else if (0 == rc) {
			if (1 == n)
				return 0;
			else
				break;
		}
		else {
			if (EINTR == errno)
				goto again;
			return (-1);
		}
	}
	*ptr = '\0';
	return n;
} /* end of Readline */

int Send(SOCKET s, const char FAR *buffer, int len, int flags)
{
	ssize_t		n;

	if (SOCKET_ERROR == (n = send(s, buffer, len, flags)))
		sock_err_sys_q("Send error");
	return n;
} /* end Send */

int sendn(SOCKET s, const char FAR *buffer, int len, int flags)
{
	size_t		bytestowrite;
	ssize_t		byteswritten;
	const char FAR *ptr = NULL;

	for (ptr = buffer, bytestowrite = len;
		bytestowrite > 0;
		ptr += byteswritten, bytestowrite -= byteswritten) {
		byteswritten = send(s, ptr, bytestowrite, 0);
		if (-1 == byteswritten && errno != EINTR)
			return -1;
		if (-1 == byteswritten)
			byteswritten = 0;
	}
	return len;
} /* end sendn */

int Sendn(SOCKET s, const char FAR *buffer, int len, int flags)
{
	int		n;

	if ((n = sendn(s, buffer, len, flags)) != len)
		sock_err_sys_q("Sendn error");
	return n;
} /* end Sendn */

int Recv(SOCKET s, char FAR *buffer, int len, int flags)
{
	ssize_t		n;

	if (SOCKET_ERROR == (n = recv(s, buffer, len, flags)))
		sock_err_sys_q("Recv error");
	return n;
} /* end Recv */

int recvn(SOCKET s, char FAR *buffer, int len, int flags)
{
	size_t		bytestoread;
	ssize_t		bytesread;
	char FAR	*ptr = NULL;

	for (ptr = buffer, bytestoread = len;
		 bytestoread > 0;
		 ptr += bytesread, bytestoread -= bytesread) {
		if ((bytesread = recv(s, ptr, bytestoread, 0)) < 0) {
			if (EINTR == errno)
				bytesread = 0;
			else
				return -1;
		} else if (0 == bytesread)
			break;
	}
	return (len - bytestoread);
} /* end recvn */

int Recvn(SOCKET s, char FAR *buffer, int len, int flags)
{
	int		n;

	if ((n = recvn(s, buffer, len, flags)) != len)
		sock_err_sys_q("Recvn error");
	return n;
} /* end Recvn */

int Recv1(int fd, char *ptr)
{
	static int read_cnt = 0;
	static char *read_ptr = NULL;
	static char line[MAXLINE];

	if (read_cnt <= 0) {
		again:
		if (-1 == (read_cnt = recv(fd, line, sizeof(line), 0))) {
			if (EINTR == errno)
				goto again;
			return -1;
		}
		else if (0 == read_cnt)
			return 0;
		read_ptr = line;
	}
	--read_cnt;
	*ptr = *read_ptr++;
	return 1;
}

int _Open(const char *filename, int oflag, int pmode)
{
	int		fd;

	if (-1 == (fd = _open(filename, oflag, pmode)))
		std_err_sys_q("_open error");
	return fd;
} /* end of _Open */

void _Close(int fd)
{
	if (-1 == _close(fd))
		std_err_sys_q("_close error");
} /* end _Close */

void Connect(SOCKET s, const struct sockaddr FAR *name, int namelen)
{
	if (SOCKET_ERROR == connect(s, name, namelen))
		sock_err_sys_q("connect error");
} /* end Connect */

FILE *Fopen(const char *filename, const char *mode)
{
	FILE	*fp = NULL;

	if (NULL == (fp = fopen(filename, mode)))
		std_err_sys_q("fopen error");
	return fp;
} /* end Fopen */

void Fclose(FILE *stream)
{
	if (EOF == fclose(stream))
		std_err_sys_q("fclose error");
} /* end Fclose */
