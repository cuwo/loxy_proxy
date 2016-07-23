#pragma once
#include "includes.h"
#include "lpx_list.h"
#include "lpx_mem.h"
#include "lpx_dbg.h"

#define LPX_SD_SIZE 65536
#define LPX_SD_HOST_SIZE 1024
#define LPX_SD_IN_SIZE 8096
#define LPX_SD_MAX_EST 1024
#define LPX_SD_TEMP_SIZE 8096 + LPX_SD_MAX_EST
#define LPX_SD_HTTP_BUF_SIZE (LPX_SD_SIZE - offsetof(SD, http_out_data))
#define LPX_SD_CLEAN (offsetof(SD, http_out_data))
#define LPX_SIGNAL SIGRTMIN

#define LPX_SD_FROM_OFFSET(ptr, offset) ((SD*)( ((char*)(ptr)) - offset ))
#define LPX_SD_FROM_MAIN_LIST(ptr) LPX_SD_FROM_OFFSET(ptr, offsetof(SD, sd_list))
#define LPX_SD_FROM_PP_LIST(ptr) LPX_SD_FROM_OFFSET(ptr, offsetof(SD, pp_list))
#define LPX_SD_FROM_DNS_LIST(ptr) LPX_SD_FROM_OFFSET(ptr, offsetof(SD, dns_list))

//all kinds of flags
#define LPX_FLAG_OPEN 1 //socket is opened
#define LPX_FLAG_CLIENT 2 //client-side, must be always connected
#define LPX_FLAG_SERVER 4 //server-side, must be always non-http
#define LPX_FLAG_HTTP 8 //produce http request to parse, only client
#define LPX_FLAG_CONN 16 //connected
#define LPX_FLAG_WAIT 32 //waiting for DNS resolve (client), or for connection (server)
#define LPX_FLAG_DNS 64 //DNS is already resolved, host line is filled (client only)
#define LPX_FLAG_FINWR 128 //must be closed after writing all the data in buffer
#define LPX_FLAG_DEAD 256 //socket is dead, don't process it
#define LPX_FLAG_MAINT 512 //mainternance socket (timer, listen, signal)
#define LPX_FLAG_SIGFD 1024 //signalfd socket
#define LPX_FLAG_TIMER 2048 //timer socket
#define LPX_FLAG_PP 4096 //requires post-processing
#define LPX_FLAG_TGET 8192 //get request type
#define LPX_FLAG_TPOST 16384 //post request type
#define LPX_FLAG_TCON 32768 //connect request type
#define LPX_FLAG_LISTEN 65536 //listen socket
#define LPX_FLAG_PP_KILL 131072 //post-processing, kill
#define LPX_FLAG_PP_WRITE 262144 //post-processing, write
#define LPX_FLAG_PP_READ 524288 //post-processing, read
#define LPX_FLAG_PP_ALL (LPX_FLAG_PP | LPX_FLAG_PP_KILL | LPX_FLAG_PP_WRITE | LPX_FLAG_PP_READ | LPX_FLAG_PP_HTTP)
#define LPX_FLAG_AUTH 1048576 //authentificated
#define LPX_FLAG_LOCK 2097152 //requires locking
#define LPX_FLAG_PP_HTTP 4194304
#define LPX_FLAG_PARSE_DEL 8388608
//main data structure in the project

typedef struct LpxBuf
{
    int ptr;
    int size;
    char data[];
} LpxBuf;

typedef union SocketDescriptor
{
    char padding[LPX_SD_SIZE];
    struct 
    {
        LpxList sd_list; //main sd list entry
        LpxList dns_list; //dns resolving list entry
        LpxList pp_list; //list of all SDs to be processed after main epoll cycle
        unsigned int flags; //SD status, type, etc
        int fd; //file descriptor, for read/write/close
        int efd; //epoll file descriptor, for control
        struct timeval ts; //timestamp, for timeout monitoring
        union SocketDescriptor * other; //other side of link
        struct gaicb dns_gai; //getaddrinfo_a arguments
        struct sigevent dns_sev; //sigevent structure for dns resolving
        struct sockaddr_in src_adr; //source address of the request
        struct sockaddr_in dst_adr; //target local address where req came to
        struct sockaddr_in trg_adr; //target address where to connect
        char host[LPX_SD_HOST_SIZE]; //target host string
        int host_size;
        int port;
        char service[6]; //target port string
        int http_limit; //in case of POST request, for mode switch
        int http_parse_ptr;
        //temp buffer (for parsed http request)
        union
        {
            struct
            {
                int http_temp_ptr;
                int http_temp_size;
                char http_temp_data[LPX_SD_TEMP_SIZE];
            };
            LpxBuf http_temp_buf;
        };
        //input buffer (for http request)
        union
        {
            struct
            {
                int http_in_ptr;
                int http_in_size;
                char http_in_data[LPX_SD_IN_SIZE];
            };
            LpxBuf http_in_buf;
        };
        //outbut buffer data
        union
        {
            struct
            {
                int http_out_ptr;
                int http_out_size;
                char http_out_data[];
            };
            LpxBuf http_out_buf;
        };
    };
} SD;

void LpxSdSetFlag(SD * sda, unsigned int flag);

void LpxSdClearFlag(SD * sda, unsigned int flag);

//check if any of flags passed are set
int LpxSdGetFlag(SD * sda, unsigned int flag);

//check if all of flags passed are set
int LpxSdGetFlags(SD * sda, unsigned int flags);

SD * LpxSdCreate();

void LpxSdClose(SD * sda);

void LpxSdDestroyAll();

void LpxSdDestroy(SD * sda);

void LpxSdInit(SD * sda, int socket, unsigned int flags, int epoll_fd, unsigned int epoll_flags);

//updates the timestamp in the desctiptor
//returns amount of millisecons since the last update
int LpxSdUpdateTimestamp(SD * sda);

int LpxSdUpdateTimestampExplicit(SD * sda, struct timeval * tv);
