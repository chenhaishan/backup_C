#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <stdarg.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <getopt.h>
#include "socket_rw_wrapper.h"
#include "thread_lib.h"
#include "ftpd.h"
#include "error_handle.h"
#include "record.h"

extern log_t *logfd;

//ftpd usage
static void ftpd_usage(void);

//ftpd verbose
static void ftpd_verbose(void);

//ftpd help
static void ftpd_help(void);

//
static void ftpd_sig_chld(int signo);

//
static void ftpd_sig_int(int signo);

//
static void ftpd_chld_sig_quit(int signo);

//
static int ftpd_ctrl_conn_handler(int ctrlfd);

//
static const char *ftpd_serv_resp_num2msg(int num);

//
int ftpd_send_resp(int ctrlfd, int num, ...);

//
static int ftpd_do_request(int ctrlfd, char *buff);

//
static int ftpd_do_auth(int ctrlfd, char *cmd);

//
static int ftpd_do_user(int ctrlfd, char *cmd);

//
static int ftpd_do_pass(int ctrlfd, char *cmd);

//
static int ftpd_do_pwd(int ctrlfd, char *cmd);

//
static int ftpd_do_cwd(int ctrlfd, char *cmd);

//
static int ftpd_do_list(int ctrlfd, char *cmd);

//
static int ftpd_do_syst(int ctrlfd, char *cmd);

//
static int ftpd_do_size(int ctrlfd, char *cmd);

//
static int ftpd_do_dele(int ctrlfd, char *cmd);

//
static int ftpd_do_rmd(int ctrlfd, char *cmd);

//
static int ftpd_do_retr(int ctrlfd, char *cmd);

//
static int ftpd_do_stor(int ctrlfd, char *cmd);

//
static int ftpd_do_pasv(int ctrlfd, char *cmd);

//
static int ftpd_do_nlst(int ctrlfd, char *cmd);

//
static int ftpd_do_port(int ctrlfd, char *cmd);

//
static int ftpd_do_type(int ctrlfd, char *cmd);

//
static int ftpd_do_quit(int ctrlfd, char *cmd);

//
static int ftpd_do_mkd(int ctrlfd, char *cmd);

//
static int ftpd_get_port_mode_ipport(char *cmd, in_addr_t *ip, uint16_t *port);

//
static int ftpd_get_connfd(void);

//
static int ftpd_get_list_stuff(char buff[], size_t len);

//
static int get_file_info(const char *filename, char buff[], size_t len);

//
static int dir_path_ok(const char *dir_path);

//
static void ftpd_close_all_fds(void);

//
static void parent_atlast(void);

//
static void pr_cpu_time(void);

//
static void ptransfer(const char *direction, long bytes, const struct timeval *t0, const struct timeval *t1);

//
static int add2pids(pid_t pid);

//
static void dele_from_pids(pid_t pid);


//flags
int ftpd_debug_on;
int ftpd_record_on;
int ftpd_quit_flag;

//hash means '#'. print '#' for each buffer transferred
int ftpd_hash_print;
//tick means print byte counter during transferring
int ftpd_tick_print;
//command service port , default 21.
int ftpd_serv_port;
//current directory
int ftpd_cur_dir[PATH_MAX];

//in passive mode, fd associate with files, connfd associate with socket.
int ftpd_cur_pasv_fd;
int ftpd_cur_pasv_connfd;
//port fd?
int ftpd_curr_port_fd;
//active or passive
int ftpd_cur_type;
//current user.
const struct ftpd_user_st *ftpd_cur_user;

//children number of parent
int ftpd_nchild;
//child process array.
pid_t pids[MAX_CHILD_NUM];

//ftpd commands and correspond recall functions
const struct ftpd_cmd_st ftpd_cmds[] = {
    { "AUTH", ftpd_do_auth },
    { "USER", ftpd_do_user },
    { "PASS", ftpd_do_pass },
    { "PWD",  ftpd_do_pwd  },
    { "XPWD", ftpd_do_pwd  },
    { "CWD",  ftpd_do_cwd  },
    { "LIST", ftpd_do_list },
    { "MKD",  ftpd_do_mkd  },
    { "XMKD", ftpd_do_mkd  },
    { "SYST", ftpd_do_syst },
    { "SIZE", ftpd_do_size },
    { "DELE", ftpd_do_dele },
    { "RMD",  ftpd_do_rmd  },
    { "TYPE", ftpd_do_type },
    { "RETR", ftpd_do_retr },
    { "STOR", ftpd_do_stor },
    { "NLST", ftpd_do_nlst },
    { "PASV", ftpd_do_pasv },
    { "PORT", ftpd_do_port },
    { "QUIT", ftpd_do_quit },
    { NULL, NULL },
};

//ftpd users:password
const struct ftpd_user_st ftpd_users[] ={
    { "anonymous", "" },
    { "ftp", "" },
    { "hexoct", "123456" }
};

/*
 * ftpd server response.
 * FTPD_LINE_END : "\r\n"
 * FTPD_VER : "1.0.1"
*/
const char ftpd_serv_resps[][256] = {
    "150 %s" FTPD_LINE_END,
    "200 %s" FTPD_LINE_END,
    "213 Size of \"%s\" is %ld." FTPD_LINE_END,
    "215 Linux Type." FTPD_LINE_END,
    "220 Ftpd" FTPD_VER " ready for new user." FTPD_LINE_END,
    "221 Goodbye." FTPD_LINE_END,
    "226 %s" FTPD_LINE_END,
    "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)." FTPD_LINE_END,
    "230 Login successful." FTPD_LINE_END,
    "250 Directory successfully changed." FTPD_LINE_END,
    "257 %s" FTPD_LINE_END,
    "331 %s" FTPD_LINE_END,
    "350 %s" FTPD_LINE_END,
    "500 %s" FTPD_LINE_END,
    "530 %s" FTPD_LINE_END,
    "550 %s" FTPD_LINE_END
};

/*
 * printf debug informations
*/
void ftpd_debug(const char *file, int line, const char *fmt, ...){
    if(ftpd_debug_on){
        va_list ap;
        int off = 0;
        char buff[MAXLINE];

        off = snprintf(buff, sizeof(buff), "%s:%d:%ld", file , line, (long)getpid());
        va_start(ap, fmt);
        vsnprintf(buff + off, sizeof(buff) - off, fmt, ap);
        va_end(ap);
        //make sure fprintf don't be disturbed
        my_lock_mutex_wait();
        fprintf(stderr, buff);
        my_lock_release();

    }
}

/*
 *
*/
void ftpd_init(void){
    int i;

    //initialize them with default value.
    ftpd_debug_on = OFF;
    ftpd_record_on = OFF;
    ftpd_hash_print = OFF;
    ftpd_tick_print = OFF;
    //if quit_flag is ture, then terminate the process
    ftpd_quit_flag = 0;
    ftpd_serv_port = SERV_PORT;/*defined in socket_rw_wrapper.h, 9877*/

    ftpd_cur_port_fd = -1;//port sockfd?
    ftpd_cur_passv_fd = -1;//passive sockfd
    ftpd_cur_pasv_connfd = -1;//?what's this
    ftpd_cur_type = TYPE_A;//default use ACTIVE mode

    /*pids[MAX_CHLD_NUM] stores parent's pid in pids[0], ohters are child's pid*/
    pids[0] = getpid();
    for(i =  1; i < MAX_CHILD_NUM; ++i){
        pids[i] = -1;
    }
    ftpd_nchild = 0;//number of children

    /*
     * when the other side is closed, and can't be wirtten any data,
     * continue call write will cause SIGPIPE, and return an EPIPE error. we can ignore it
     * in case of terminate the process(default action of reception of SIGPIPE), and capture
     * errno = EPIPE to handle.
    */
    Signal(SIGPIPE, SIG_IGN);

    /*
     * install SIGCHLD's handler for 4 reasons:
     * 1.waitpid(child), free resource of child
     * 2.check child's return status
     * 3.remove child from pids[MAX_CHILD_NUM]
     * 4.print information about child's live time.
    */
    Signal(SIGCHLD, ftpd_sig_chld);

    /*
     * when capture 'Ctrl + C',
     * send SIGQUIT to all children in pids[MAX_CHILD_NUM],
     * and exit.
    */
    Signal(SIGINT, ftpd_sig_int);

    /*initialize the mutex-lock. map a anonymous memory to share with process as a mutex*/
    my_lock_mutex_init();

    /*
     * register an function to be called at the termination of process.
     * parent_atlast will check wether file descriptor of log file is on, and will closed.
    */
    atexit(parent_atlast);
}

/*
 *handle the args
 *
 * -d/--debug --> open debug mode
 * -p/--port [port#] -->specify the listen port (9877 as default)
 * -r/--record["filename"] -->open log mode("logfile.txt" as default)
 * -h/--help --> show help
 * -v/--verbose -->show version
 *
 * @argc:
 * @argv: the args passed to main func
*/
void ftpd_parse_args(int argc, char **argv){
    //flags
    int do_verbose, do_help;
    int err_flg;

    char c, log_filename[PATH_MAX];
    /*check getopt_long manpage*/
    struct option longopts[] = {
        {"port", required_argument, NULL, 'p'},
        {"debug", no_argument, NULL, 'd'},
        {"record", optional_argument, NULL, 'r'},
        {"verbose", no_argument, &do_verbose, 1},
        {"help", no_argument, &do_help, 1},
        {0, 0, 0, 0}
    };

    do_verbose = OFF;
    do_help = OFF;
    err_flg = 0;

    while((c = getopt_long(argc, argv, ":hr::vp:dW;", longopts, NULL)) == -1){
        switch(c){
        case 'd':
            ftpd_debug_on = ON;
            ftpd_hash_print = ON;
            ftpd_tick_print = ON;
            break;
        case 'r':
            strcpy(log_filename, optarg?optarg:FTPD_DEF_LOGFILE);
            ftpd_record_on = ON;
            break;
        case 'v':
            do_verbose = ON;
            break;
        case 'p':
            if (0 == (ftpd_serv_port = (uint16_t) atoi(optarg)))
                err_flg = 1;
            break;
        case 0:
            break;
        case ':':
            err_msg("%s: option `-%c' requires an argument",
                    argv[0], optopt);
            err_flg = 1;
            break;
        case '?':
        default:
            err_msg("%s: %c: unknow option", argv[0], optopt);
            err_flg = 1;
            break;
        }
    }

    if(err_flg){
        ftpd_usage();
        exit(EXIT_FAILURE);
    }else if(do_help){
        ftpd_help();
        exit(EXIT_SUCCESS);
    }else if(do_verbose){
        ftpd_verbose();
        exit(EXIT_SUCCESS);
    }

    if(ftpd_record_on){
        if(NULL == (logfd = log_open(log_filename, LOG_DEFAULT))){
            FTPD_DEBUG("Cannot oopen logfile \"%s\"", log_filename);
            exit(EXIT_FAILURE);
        }
        FTPD_DEBUG_LOG(INFO, "Prepare to log in \"%s\" ok.\n",log_filename);
    }

    FTPD_DEBUG("serv port %u\n", ftpd_serv_port);
}

/*show usage when an error occurs*/
static void ftpd_usage(void){
    printf("usage: ftpd_main [-p <port#>] [-r[\"filename\"]][-v] [-d] [-h]\n");
}

/*show vesion*/
static void ftpd_verbose(void)
{
    printf("A simple ftp server.\n"
           "It can support some of the mostly used cmds.\n"
           "Author: jiangweifeng\n"
           "Mail: jwf0713@gmail.com\n"
           "Ftpd version: 1.1\n");
}

/*
 * show help
 */
static void ftpd_help(void)
{
    printf("You can use any ftp client software to login.\n"
           "Cmds as follows are supported:\n"
           "user ls cd pwd get mget delete put mput\n"
           "size port passive ascii binary dir mkdir\n"
           "......\n"
           "Well, you can add more cmds!\n");
}

/*parent process create a listen fd*/
int ftpd_create_serv(void){
    int listenfd;
    const int on = 1;
    struct sockaddr_in servaddr;

    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    Setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(ftpd_serv_port);
    Bind(listenfd, (SA *)&servaddr, sizeof(servaddr));

    Listen(listenfd, LISTENQ);

    FTPD_DEBUG_LOG(INFO, "create serv-listenfd ok %s\n",
                   Sock_ntop((SA *)&servaddr, sizeof(servaddr)));
    return listenfd;
}

/*main loop, server is always listening and fork a child to handle the new client*/
int ftopd_do_loop(int listenfd){
    int crtlfd;
    pid_t childpid;

    for(;;){
        FTPD_DEBUG("server is ready for a new connection ...\n");
        if(-1 == (ctrlfd = accept(listenfd, NULL, NULL))){
            FTPD_DEBUG_LOG(ERROR, "accept failed: %s\n", strerror(errno));
            continue;
        }

        if(ftpd_debug_on){

        }
    }

}




































