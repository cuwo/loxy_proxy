//main proxy source file. everything else comes from here.
#define _GNU_SOURCE
#include "includes.h"
#include "lpx_mt.h"
#include "lpx_args.h"
#include "lpx_init.h"
#include "lpx_dbg.h"
#include "lpx_cb.h"
//saved password auth info
LpxConstString LpxGlobalPassData = {NULL, 0};
char * temp_buf = NULL;

int main(int argc, char ** argv)
{
    struct epoll_event *events;
    struct timeval cycle_time;
    SD * sda;
    LpxList * list_elem;
    int listen_port, epoll_fd, i, n, temp;
    extern LpxList LpxSdGlobalListPP;
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
    events = (struct epoll_event*)LpxMemSafeAlloc (LPX_MAX_EVENTS * sizeof(struct epoll_event));
    if(events == NULL)
    {
        printf("Event buffer alloc failed!\n");
        exit(-1);
    }
    //enter main cycle
    while(1)
    {
        n = epoll_wait (epoll_fd, events, LPX_MAX_EVENTS, -1);
        gettimeofday(&cycle_time, NULL);
        for (i = 0; i < n; ++i)
        {
            sda = (SD*)(events[i].data.ptr);
            //do callbacks depending on what we got here
            if (LpxSdGetFlag(sda, LPX_FLAG_MAINT))
            {
                //mainternance
                if (LpxSdGetFlag(sda, LPX_FLAG_LISTEN)) //new client connected
                {
                    LpxCbAccept(sda);
                }
                else if (LpxSdGetFlag(sda, LPX_FLAG_TIMER)) //periodic timer, check all connected clients
                {
                
                }
                else //DNS processing
                {
                
                }
            }
            else
            {
                //timeout control
                LpxSdUpdateTimestampExplicit(sda, &cycle_time);
                //data
            }
        }
        
        //perform PP
        list_elem = LpxSdGlobalListPP.next;
        while(list_elem != NULL)
        {
            sda = LPX_SD_FROM_PP_LIST(list_elem);
            //do callbacks depending on PP task
            
            //process next element
            LpxListRemove(list_elem);
            list_elem = LpxSdGlobalListPP.next;
        }
    }
}
