#ifndef _DXYH_H
#define _DXYH_H

#include <Winsock2.h>

#define SERV_PORT		9876
#define BACKLOG			20
#define Isspace(c)		(((' ' == (c)) || (((unsigned int)((c) - 9)) <= (13 - 9))))
#define Isprint(c)		(((unsigned int)((c) - 0x20)) <= (0x7e - 0x20))

typedef SOCKADDR	SA;
typedef signed int	ssize_t;

SOCKET Socket(int af, int type, int protocol);
void Bind(SOCKET s, const struct sockaddr FAR *name, int namelen);
void Listen(SOCKET s, int backlog);
SOCKET Accept(SOCKET s, struct sockaddr FAR *addr, int FAR *addrlen);
int _Write(int fd, const void *buffer, unsigned int count);
int _Read(int fd, void *buffer, unsigned int count);
int Writen(int fd, const void *buffer, unsigned int n);
int Readn(int fd, void *buffer, unsigned int n);
int Recv1(int fd, char *ptr);
int Readline(int fd, void *vptr, unsigned int maxlen);
int Read1(int fd, char *ptr);
int _Open(const char *filename, int oflag, int pmode);
void _Close(int fd);
void Connect(SOCKET s, const struct sockaddr FAR *name, int namelen);
int Send(SOCKET s, const char FAR *buffer, int len, int flags);
int Sendn(SOCKET s, const char FAR *buffer, int len, int flags);
int Recv(SOCKET s, char FAR *buffer, int len, int flags);
int Recvn(SOCKET s, char FAR *buffer, int len, int flags);
FILE *Fopen(const char *filename, const char *mode);
void Fclose(FILE *stream);
#endif
