#ifndef FTPD_H
#define FTPD_H

//FTPD ARRAY LENGHT
#define FTPD_ARR_LEN(arr) (sizeof(arr) / sizeof(arr[0]))
//FTPD VERSION
#define FPTD_VER "1.0.1"
//FTPD DEFAULT SERVICE PORT
#define FTPD_DEF_SERV_PORT 21
//FTPD LINE END
#define FTPD_LINE_END "\r\n"
//FTPD OK
#define FTPD_OK 0
//FTPD ERROR
#define FTPD_ERR (-1)
//FTPD DEFAULT LOG FILE
#define FTPD_DEF_LOGFILE "logfile.txt"
//ON
#define ON 1
//OFF
#define OFF 0

/*ftpd transaction mode*/
//TYPE_A means ACTION mode
#define TYPE_A 0
//TYPE_I meas PASSIVE mode
#define TYPE_I 1

//HASH BYTES
#define HASHBYTES 1024
//TICK BYTES
#define TICKBYTES 10240
//MAX CHILD NUM
#define MAX_CHILD_NUM 10

#define FTPD_CHECK_LOGIN() \
    do{\
        if(NULL == ftpd_cur_user){\
            ftpd_send_resp(ctrlfd, 530, "User haven't logged in");\
            return FTPD_ERR;\
        }\
    }while(0)

#define FTPD_DEBUG(fmt, ...) \
    ftpd_debug(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define FTPD_LOG(level, fmt, ...) \
    do{\
        if(ftpd_record_on)\
            log_msg(logfd, level, fmt, ##__VA_ARGS__);\
    }while(0)

#define FTPD_DEBUG_LOG(level, fmt, ...) \
    do{\
        ftpd_debug(__FILE__, __LINE__, fmt, ##__VA_ARGS__);\
        if(ftpd_record_on)\
            log_msg(logfd, level, fmt, ##__VA_ARGS__);\
    }while(0)


/*command structure*/
struct ftpd_cmd_st{
    char *cmd;
    int (*cmd_handler)(int, char *);
};

/*user-password structure*/
struct ftpd_usr_st{
    char user[128];
    char pass[128];
};

void ftpd_parse_args(int argc, char **argv);

void ftpd_debug(const char *file, int line, const char *fmt, ...);

void ftpd_init(void);

int ftpd_create_serv(void);

int ftpd_do_loop(int listenfd);

#endif // FTPD_H
