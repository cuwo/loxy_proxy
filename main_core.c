//main proxy source file. everything else comes from here.
#define _GNU_SOURCE
#include "lpx_mt.h"
#include "lpx_args.h"
#include "lpx_init.h"
#include "lpx_dbg.h"
//saved password auth info
LpxConstString LpxGlobalPassData = {NULL, 0};
char * temp_buf = NULL;

int main(int argc, char ** argv)
{
    struct epoll_event *events;
    SD * sda;
    int listen_port, epoll_fd, i, n, temp;
    dbgprint(("debug mode enabled!\n"));
    //parse arguments
    LpxArgsParse(argc, argv, &listen_port);
    //init memory pool and alloc temp buffer
    LpxMemPoolInit(LPX_SD_SIZE);
    temp_buf = LpxMemPoolAlloc();
    //init epoll
    epoll_fd = epoll_create1(0);
    //init listen socket
    if (LpxListenCompleteInit(listen_port, epoll_fd) != 0)
    {
        printf("Listen init failed!\n");
        exit(-1);
    }
    //init signalfd for DNS resolving
    if (LpxDnsCompleteInit(epoll_fd) != 0)
    {
        printf("DNS init failed!\n");
        exit(-1);
    }
    //init mainternance timer
    if (LpxTimerCompleteInit(epoll_fd) != 0)
    {
        printf("Timer init failed!\n");
        exit(-1);
    }
    //alloc events buffer
    events = (epoll_event*)LpxMemSafeAlloc (LPX_MAX_EVENTS * sizeof(struct epoll_event));
    if(events == NULL)
    {
        printf("Event buffer alloc failed!\n");
        exit(-1);
    }
    //enter main cycle
    while(1)
    {
        n = epoll_wait (epoll_fd, events, LPX_MAX_EVENTS, -1);
    }
}
