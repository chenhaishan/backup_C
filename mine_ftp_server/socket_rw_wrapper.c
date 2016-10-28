#include "socket_rw_wrapper.h"
#include "error_handle.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stropts.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netinet/in.h>
#include <netdb.h>

/*file lock*/
static sturct flock lock_it, unlock_it;

static int lock_fd = -1;

/*
 * wrapper for socket functions
*/

/*wrapper for socket*/
int Socket(int family, int type, int protocol){
    int n;
    if(-1 == (n = socket(family, type, protocol)))
        err_sys_q("socket error");
    return n;
}

/*wraper for socketpair*/
void Socketpair(int family, int type, int protocol, int *fd){
    int n;
    if((n = socketpair(family, type, protocol, fd)) < 0)
        err_sys_q("socketpair error");
}

/*wraper for bind*/

int Bind(int sockfd, struct sockaddr *my_addr, socklen_t addrlen){
    int n;
    if(-1 == (n = bind(sockfd, my_addr, addrlen)))
        err_sys_q("bind error");
    return n;
}

/*wraper for listen*/
int Listen(int socket, ing backlog){
    char *ptr = NULL;
    int n;
    /*get an enviroment vairalbe named "LISTENQ"*/
    if((ptr = getenv("LISTENQ")) != NULL)
        backlog = atoi(ptr);
    if(-1 == (n = listen(socket, backlog)))
        err_sys_q("listen error");
    return n;
}

/*wrapper for accept*/
int Accept(int s, sturct sockaddr *addr, socklen_t *addrlen){
    int n;
    if(-1 == (n = accept(s, addr, addrlen)))
        err_sys_q("accept error");
    return n;
}

/*wrapper for inet_ntop, which converts the network address structure src in the af address family
  into a chararcter string. the resulting string is copied to the buffer pointed to by dst, which
  must be a non-null pointer. the caller speifies the number of bytes avaiable in this buffer in the
  argument size.
  for AF_INET, 4bytes(32bits) -> ddd.ddd.ddd.ddd
  for AF_INET6, 8bytes(64bits) -> x:x:x:x:x:x:x:x:x
*/
const char *Inet_ntop(int af, const void *src, char *dst, socklen_t size){
    const char *ptr = NULL;
    if(NULL == src)
        err_msg_q("NULL 3rd argument to inet_ntop");
    if(NULL == (ptr = inet_ntop(af, src, dst, size)))
        err_sys_q("inet_ntop error");
    return ptr;
}

/*wrapper for inet_pton, which converts the character string src into a network address structure
  in the af address fanily, then copies the network address sturcture to dst.
  for AF_INET, ddd.ddd.ddd.ddd -> 4 bytes (32bits)
  for AF_INET6, x:x:x:x:x:x:x:x:x -> 8bytes (64bits)
*/
int Inet_pton(int af, const char *src, void *dst){
    int n;
    if(-1 == (n = inet_pton(af, src, dst)))
        err_sys_q("inet_pton error");
    else if(0 == n)
        err_msg_q("inet_pton error for %s", src);
    return n;
}

/*wrapper for inet_addr, convert Ineternet IP host address from IPv4 numbers-and-dots notation
into binary data in network byte order
*/
in_addr_t Inet_addr(const char *cp){
    in_addr_t s_addr;
    if((in_addr_t) -1 == (s_addr = inet_addr(cp)))
        err_msg_q("inet_addr error");
    return s_addr;
}

/*wrapper for inet_ntoa, convert Ineternet IP host address given in network byte order,
 * into a stirng in IPv4 numbers-and-dots notation.return a statically allocated buffer.*/
char *Inet_ntoa(struct in_addr in){
    return inet_ntoa(in);
}

/*wrapper for connect*/
int Connect(int socket, const struct sockaddr *address, socklen_t address_len){
    int n;
    if(-1 == (n = connect(socket, address, address_len)))
        err_sys_q("connect error");
    return n;
}

/*convert network address and port into binary.
  for AF_INET, ddd.ddd.ddd.ddd:port -> 4bytes + 2bytes
*/
char *sock_ntop(const strcut sockaddr *sa, socklen_t salen){
    char portstr[7];
    static char str[128];

    switch(sa->sa_family){
    case AF_INET:{
        struct sockaddr_in *sin = (struct sockaddr_in *)sa;
        Inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str));
        if(ntohs(sin->sin_port) != 0){
            snprintf(portstr, sizeof(portstr), ":%d", ntohs(sin->sin_port)));
            strcat(str, portstr);
        }
        return str;
    }break;
    case AF_INET6:{
        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
        Inet_ntop(AF_INET6, &sin6->sin6_addr, str, sizeof(str));
        if(ntohs(sin6->sin6_port) != 0){
            snprintf(portstr, sizeof(portstr), ".%d", ntohs(sin6->sin6_port));
            strcat(str, portstr);
        }
        return str;
    }break;
    default:
        snprintf(str, sizeof(str), "sock_ntop error: unknown AF_XXX: %d, len d%",
                 sa->sa_family, salen);
        return str;
    }
    return NULL;
}

/*wraper of the above function sock_ntop*/
char *Sock_ntop(const struct sockaddr *sa, socklen_t salen){
    char *ptr = NULL;
    if(NULL == (ptr = sock_ntop(sa, salen)))
        err_sys_q("sock_ntop error");
    return ptr;
}

/*non-blocking connect*/
int connect_nonb(int sockfd, const SA *saptr, socklen_t salen, int nsec){
    int flags;
    socklen_t len, n, error;
    fd_set rset, wset;
    struct timeval tv;

    //get sockfd's flag, then set it to nonblock
    flags = Fcntl(sockfd, F_GETFL, 0);
    Fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    error = 0;
    if(-1 == (n = connect(sockfd, saptr, salen)))
        /*if errno == EINPROGRESS is true, it means the socket is nonblocking and the connection can't be completed immediately.
         * it is possible to select() or poll() for completion by selecting the socket for writting. after select() indicated writability,
         * use getsockopt()(UNP, P162) to read the SO_ERROR option at level SOL_SOCKET to determine whether connect() compeleted successfully(
         * SO_ERROR is zero) or unsuccessfully(SO_ERROR is one of the usual error codes, expalining the resason for the failure)
         *
        */
        if(errno != EINPROGRESS)
            return -1;
    /*connect success*/
    if(n == 0)
        goto done;

    FD_ZERO(&rset);
    FD_SET(sockfd, &rset);
    wset = rset;
    tv.tv_sec = nsec;
    tv.tv_usec = 0;
    /*if connect can't be completed immediately, wait for tv seconds to check if sockfd is ready.*/
    if(0 == (n = Select(sockfd+1, &rset, &wset, NULL,
                        nsec ? &tv : NULL))){
        Close(sockfd);
        errno = ETIMEDOUT;//connection timed out.
        return -1;
    }
    if(FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset)){
        len = sizeof(error);
        /*if connect() compeleted successfully, error is set to zero*/
        /*Question: if sockfd is 'R/W able', is it neccessary to call getsockopt ?(UNP, P131)*/
        /*Answer: YEs! when error happens, sockfd is in readset or writeset.(UNP,P141)*/
        if(-1 == getsockopt(sockfd, SOL_SOCKET, SO_ERROR,
                            &error, &len))
            return -1;
    }else{
        err_msg_q("select error: sockfd not set");
    }

done:
    Fcntl(sockfd, F_SETFL, flags);
    /*connect() failed, error is set to one of the error codes.*/
    if(error){
        Close(sockfd);
        errno = error;
        return -1;
    }
    return 0;
}

/*wrapper for shutdown, which cause all or part of a full-duplex connection on the socket associated
 * with sockfd to be shut down. if how if SHUT_RD, futher receptions will be disallowed. if how is
 * SHUT_WR, further transmissions will be disallowed. if how if SHUT_REWR, futher receptions and
 * transmissions will be disallowed.
*/
int Shutdown(int sockfd, int how){
    if(-1 == shutdown(sockfd, how))
        err_sys_q("shutdown error");
}

/*wrapper for tcp listen for ipv4/6*/
int tcp_listen(const char *host, const char *serv, socklen_t *addrlenp){
    int listenfd, n;
    const int on = 1;
    struct addrinfo hints, *res = NULL, *ressave = res;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if((n = getaddrinfo(host, serv, &hints, &res)) != 0)
        err_msg_q("tcp_listen error for %s, %s, %s", host, serv, gai_strerror(n));
    ressave = res;
    do{
        listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if(-1 == listenfd)
            continue;
        /*allow different protocol, IPv4/6 , to use the same port*/
        Setsockopt(listenfd, SOL_SOCKET, SO_REUSERADDR, &on, sizeof(on));
        if(0 == bind(listenfd, res->ai_addr, res->ai_addrlen))
            break;
        Close(listenfd);
    }while((res = res->ai_next) != NULL);
}

/*wrappper for tcp_listen*/
int Tcp_listen(const char *host, const char *serv, socklen_t *addrlenp){
    return (tcp_listen(host, serv, addrlenp));

}

/*wrapper for getaddrinfo*/
struct addrinfo *host_serv(const char *host, const char *serv, int family, int socktype){
    int n;
    struct addrinfo hints, *res = NULL;

    bzero(&hints, sizeof(hints));
    hints.ai_flags = AI_CANONNAME;
    hints.ai_family = family;
    hints.ai_socktype = socktype;

    if((n = getaddrinfo(host, serv, &hints, &res)) != 0){
        return NULL;
    }

    return res;
}

/*wrapper for tcp connect*/
int tcp_connect(const char *host, const char *serv){
    int sockfd, n;
    struct addrinfo hints, *res = NULL, *ressave = res;

    bzero(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if((n = getaddrinfo(host, serv, &hints, &res)) != 0)
        err_msg_q("tcp_connect error for %s: %s", host, serv, gai_strerror(n));
    ressave = res;
    do{
        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if(-1 == sockfd)
            continue;
        if(0 == connect(sockfd, res->ai_addr, res->ai_addrlen))
            break;
        Close(sockfd);
    }while((res = res->ai_next) != NULL);

    if(NULL == res)
        err_msg_q("tcp_connect error for %s, %s", host, serv);

    freeaddrinfo(ressave);
    return sockfd;
}

/*wrapper for tcp_connect*/
int Tcp_connect(const char *host, const char *serv){
    return (tcp_connect(host, serv));
}

/*wrapper for setsockopt*/
void Setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen){
    if(-1 == setsockopt(fd, level, optname, optval, optlen))
        err_sys_q("setsockopt error");
}

/*wrapper for getpeername*/
void Getpeername(int fd, struct sockaddr *sa, socklen_t *salenptr){
    if(-1 == getpeername(fd, sa, salenptr))
        err_sys_q("getpeername error");
}

/*wrapper for getsockname*/
void Getsockname(int fd, struct sockaddr *sa, socklen_t *salenptr){
    if(-1 == getsockname(fd, sa, salenptr))
        err_sys_q("getsockname error");
}




/*
 * wrapper for read functions
*/

/*read n bytes to buf, return bytes read*/
/*ssize_t = signed size_t, and size_t is read/write from/to buff one time's minmum size*/
ssize_t readn(int fd, void *buf, size_t n){
    size_t bytes2read;
    ssize_t bytesread;
    char *ptr = NULL;

    for(ptr = buf, bytes2read = n;bytes2read > 0;
        ptr += bytesread, byres2tread -= bytesread){
        if((bytesread = read(fd, ptr, bytes2read)) < 0){
            /*if read interrupted by signal handler function, restart it*/
            if(EINTR == errno)
                bytesread = 0;
            else
                return -1;
        }
        else if(0 == bytesread)
            break;
    }
    return (n - bytes2read);
}

/*any difference to readn?*/
ssize_t Readn(int fd, void *buf, size_t n){
    ssize_t len;
    if(-1 == (len = readn(fd, buf, n)))
        err_sys_q("readn error");
    return len;
}

/*wrapper for read.if interrutped by signal function, n == -1 && errno == EINTR*/
ssize_t Read(int fd, void *buf, size_t len){
    ssize_t n;
    if(-1 == (n = read(fd, buf, len)))
        err_sys_q("read error");
    return n;
}

/*read 1 byte from fd with buffering*/
ssize_t Read1(int fd, char *ptr){
    static int read_cnt = 0;
    static char *read_ptr = NULL;
    static char read_buf[MAXLINE];

    if(read_cnt <= 0 ){
        again:
        if(-1 == (read_cnt = read(fd, read_buf, sizeof(read_buf)))){
            if(EINTR == errno)
                goto again;
            return -1;
        }
        else if(0 == read_cnt)
            return 0;
        read_ptr = read_buf;
    }
    --read_cnt;
    *ptr = *read_ptr++;
    return 1;
}

/*read 1 line from fd*/
ssize_t Readline(int fd, void *vptr, size_t maxlen){
    ssize_t n, rc;
    char c, *ptr = vptr;

    for(n = 1; n < maxlen; ++n){
        again:
        if(1 == (rc = Read1(fd, &c))){
            *ptr++ = c;
            if('\n' == c)
                break;
        }
        else if(0 == rc){
            if(1 == n)
                return 0;
            else
                break;
        }
        else {
            if(EINTR == errno)
                goto agian;
            return -1;
        }
    }
    *ptr = '\0';
    return n;
}

/*wrapper for fgets.fgets reads in at most one less than size chararcters from stream and stores them into the buffer pointed by s.
 * reading stops after an EOF ro a newline. if a newline is read, it is stored into thebuffer, a terminating null byte('\0') is
 * stored after the last character in the buffer.return ptr on success, and NULL on error.
*/
char *Fgets(char *ptr, int n, FILE *stream){
    char *rptr = NULL;
    if(NULL == (rptr = fgets(ptr, n, stream)) && ferror(stream))
        err_sys_q("fgets error");
    return rptr;
}






/*
 * wrapper for write functions
*/

/*write n bytes from buf to fd, return n on success*/
ssize_t writen(int fd, const void *buf, size_t n){
    size_t bytes2write;
    ssize_t byteswriten;
    const char *ptr = NULL;

    for(ptr = buf, bytes2write = n;bytes2write > 0;
        ptr += byteswriten, bytes2write -= byteswriten){
        byteswriten = write(fd, ptr, bytes2write);
        /*an error occurred*/
        if(-1 == byteswriten && errno != EINTR)
            return -1;
        /*interrupted by signal, continue to write*/
        if(-1 == byteswriten && errno == EINTR)
            byteswriten = 0;
    }

    return n;
}


/*wrapper for writen*/
ssize_t Writen(int fd, const void *buf, size_t n){
    ssize_t len;
    if(-1 == (len = writen(fd, buf, n)))
        err_sys_q("writen error");
    return len;
}

/*wrapper for write*/
ssize_t Write(int fd, const void *buf, size_t n){
    ssize_t n;
    if(-1 == (n = write(fd, buf, n)))
        err_sys_q("write error");
    return n;
}

/*wrapper for close*/
int Close(int fd){
    int n;
    if(-1 == (n = close(fd)))
        err_sys_q("close error");
    return n;
}

/*wrapper for fputs*/
void Fputs(const char *ptr, FILE *stream){
    if(EOF == fputs(ptr, stream))
        err_sys_q("fputs error");
}


/*
 * wrapper for other system call
*/

/*wrapper for fork*/
pid_t Fork(void){
    pid_t pid;
    if(-1 == (pid = fork()))
        err_sys_q("fork error");
    return pid;
}

/*wrapper for vfork.It is used to create new processes without copying the
 *page tables of the parent process. It may be useful in performance-sensitive applications where
 *a child is created which then immediately issues an execve(2).
*/
pid_t Vfork(void){
    pid_t pid;
    if(-1 == (pid = vfork()))
        err_sys_q("vfork error");
    return pid;
}

/*wrapper for popen, which open a process by creating a pipe ,forking, and invoking the shell.*/
FILE *Popen(const char *command, const char *mode){
    FILE *fp;
    if(NULL == (fp = popen(command, mode)))
        err_sys_q("popen error");
    return fp;
}

/*wrapper for pclose*/
void Pclose(FILE *stream){
    if(-1 == pclose(stream))
        err_sys_q("pclose error");
}

/*wrapper for select*/
int Select(int nfds, fd_set *readfds, fd_set *writefds,
           fd_set *exceptfds, struct timeval *timeout){
    int n;
    if(-1 == (n = select(nfds, readfds, writefds, exceptfds, timeout)))
        err_sys_q("select error");
    return n;
}


/*wrapper for wait, suspend calling process unitl child process terminated*/
pid_t Wait(int *iptr){
    pid_t pid;
    if(-1 == (pid = wait(iptr)))
        err_sys_q("wait error");
    return pid;
}

/*wrapper for waitpid, which suspend calling process until a child process specified by pid has changed state.*/
pid_t Waitpid(pid_t pid, int *iptr, int options){
    pid_t retpid;
    if(-1 == (pid = waitpid(retpid, iptr, options)))
        err_sys_q("waitpid error");
    return retpid;
}




/*wrapper for calloc: allocates memory for an arrary fo n member elements of size bytes each, and returns a pointer to the allocated memeory*/
void Calloc(size_t n, size_t size){
    void *ptr;
    if(NULL ==(ptr = calloc(n, size)))
        err_sys_q("calloc error");
    return ptr;
}


/*wrapper for dup2.create a copy of fd1 with specificed file descriptor fd2*/
void Dup2(int fd1, int fd2){
    if(-1 == dup2(fd1, fd2))
        err_sys_q("dup2 error");
}

/*wrapper fro gettimeofday*/
void Gettimeofday(struct timeval *tv, void *foo){
    if(-1 == (gettimeofday(tv, foo)))
        err_sys_q("gettimeofday eroor");
}

/*wrapper for ioctl*/
int Ioctl(int fd, int request, void *arg){
    int n;
    if(-1 == (n = ioctl(fd, request, arg)))
        err_sys_q("ioctl error");
    return n;
}

/**wrapper for open*/
int Open(const char *pahtname, int oflag, mode_t mode){
    int fd;
    if(-1 == (fd = open(pathname, oflag, mode)))
        err_sys_q("open error");
    return fd;
}

/*wrapper for unlink, which delete a file from file system*/
void Unlink(const char *pathname){
    if(-1 == unlink(pathname))
        err_sys_q("unlink error");
}

/*wrapper for fopen*/
FILE *Fopen(const char *pathname, const char *mode){
    FILE *fp;
    if(NULL == (fp = fopen(pathname, mode)))
        err_sys_q("fopen error");
    return fp;
}

/*wrapper for fclose*/
FILE Fclose(FILE *fp){
    if(fclose(fp) != 0)
        err_sys_q("fclose error");
}

/*wrapper for fcntl*/
int Fcntl(int fd, int cmd, int arg){
    int n;
    if(-1 == (n = fcntl(fd, cmd, arg)))
        err_sys_q("fcntl error");
    return n;
}

/*wrapper for malloc*/
void *Malloc(size_t size){
    void *ptr = NULL;
    if(NULL == (ptr = malloc(size)))
        err_sys_q("malloc error");
    return ptr;
}

/*wrapper for getcwd*/
char *Getcwd(char *buf, size_t size){
    char *ptr;
    if(NULL == (ptr = getcwd(buf, size)))
        err_sys_q("getcwd error");
    return ptr;
}

/*wrapper for mkdir */
void Mkdir(const char *path, mode_t mode){
    if(-1 == mkdir(ptah, mode))
        err_sys_q("mkdir error");

}

/*wrapper for rmdir*/
void Rmdir(const char *path){
    if(-1 == rmdir(path))
        err_sys_q("rmdir error");
}

/*wrapper for chdir*/
void Chdir(const char *path){
    if(-1 == chdir(path))
        err_sys_q("chdir error");
}

/*wrapper for opendir*/
DIR *Opendir(const char *dirname){
    DIR *dirp;
    if(NULL == (dirp = opendir(dirname)))
        err_sys_q("opendir error");
    return dirp;
}

/*wrapper for closedir. like FILE *, DIR * also should be closed. */
void Closedir(DIR *dirp){
    if(-1 == closedir(dirp))
        err_sys_q("closedir error");
}

/*wrapper for mkstemp. In Linux, should be
int mkstemp(char *template);
which use template to create an unique temporary file name, create and open it, and return
file descriptor.
So this function maybe not correct.
*/
void Mkstemp(char *temp){
    if(-1 == mkstemp(temp) || 0 == temp[0] )
        err_msg_q("mkstemp error");
}

/*wrapper for mmap*/
void *Mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset){
    void *ptr;
    if((void *) -1 == (ptr = mmap(addr, len , prot, flags, fd, offset)))
        err_sys_q("mmape error");
    return ptr;
}


/*
 * wrapper for signal functions
*/

/*wrapper for signal*/
Sigfunc * Signal(int signo, Sigfunc *func){
    Sigfunc *sigfunc;
    if(SIG_ERR == (sigfunc = signal(signo, func)))
        err_sys_q("signal error");
    return sigfunc;
}

/*wrapper for kill*/
void Kill(pid_t pid, int sig){
    if(-1 == kill(pid, sig))
        err_sys_q("kill error");
}




/*
 * wrapper for lock
*/

/**/
void my_lock_wait(void){
    int rc;
    while((rc = fcntl(lock_fd, F_SETLKW, &lock_it)) < 0){
        if(EINTR == errno)
            continue;
        else
            err_sys_q("fcntl error for my_lock_wait");
    }
}

/* */
void my_lock_release(void){
    if(fcntl(lock_fd, F_SETLKW, &unlock_it) < 0)
        err_sys_q("fcntl error for my_lock release");
}

/**/



























































