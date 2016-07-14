#pragma once
#include "includes.h"
#include "lpx_list.h"
#include "lpx_mem.h"

#define LPX_SD_SIZE 65536
#define LPX_SD_HOST_SIZE 1024
#define LPX_SD_IN_SIZE 8096
#define LPX_SD_BUF_SIZE (LPX_SD_SIZE - offsetof(union SocketDescriptot, out_data))
#define LPX_SD_HTTP_BUF_SIZE (LPX_SD_SIZE - offsetof(union SocketDescriptot, http_out_data))
#define LPX_SD_CLEAN (offsetof(union SocketDescriptot, http_out_data))

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
#define LPX_FLAG_GET 8192 //get request
#define LPX_FLAG_POST 16384 //post request
#define LPX_FLAG_CONNECT 32768 //connect request
#define LPX_FLAG_CLRGP !(LPX_FLAG_GET | LPX_FLAG_POST) //to clear get/post flags
#define LPX_FLAG_LISTEN 65536 //listen socket

//main data structure in the project

typedef union SocketDescriptor
{
    char padding[LPX_SD_SIZE];
    struct 
    {
        LpxList sd_list; //dns resolving list entry
        LpxList dns_list; //dns resolving list entry
        LpxList post_list; //list of all SDs to be processed after main epoll cycle
        unsigned int flags; //SD status, type, etc
        int fd; //file descriptor, for read/write/close
        int efd; //epoll file descriptor, for control
        struct timeval ts; //timestamp, for timeout monitoring
        union SocketDescriptor * other; //other side of link
        union
        {
            struct //for direct sockets (server, or connected client)
            {
                //only output data buffer
                int out_ptr;
                int out_size;
                char out_data[];
            };
            struct //for http sockets (client)
            {
                struct gaicb dns_gai; //getaddrinfo_a arguments
                struct sigevent dns_sev; //sigevent structure for dns resolving
                struct sockaddr_in src_adr; //source address of the request
                struct sockaddr_in dst_adr; //target local address where req came to
                struct sockaddr_in trg_adr; //target address where to connect
                char host[LPX_SD_HOST_SIZE]; //target host string
                char service[6]; //target port string
                //reserved ints, will be renamed later
                int http_temp1;
                int http_temp2;
                //input buffer data
                int http_in_ptr;
                int http_in_size;
                char http_in_data[LPX_SD_IN_SIZE];
                //outbut buffer data
                int http_out_ptr;
                int http_out_size;
                char http_out_data[];
            };
        };
    };
} SD;

void LpxSdSetFlag(SD * sda, unsigned int flag);

void LpxSdClearFlag(SD * sda, unsigned int flag);

int LpxSdGetFlag(SD * sda, unsigned int flag);

SD * LpxSdCreate();

void LpxSdClose(SD * sda);

void LpxSdDestroyAll();

void LpxSdDestroy(SD * sda);

//updates the timestamp in the desctiptor
//returns amount of millisecons since the last update
int LpxSdUpdateTimestamp(SD * sda);
