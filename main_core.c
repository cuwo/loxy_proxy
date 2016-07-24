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
    //parse arguments
    LpxArgsParse(argc, argv, &listen_port);
    dbgprint(("debug mode enabled!\n"));
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
    //init error messages
    LpxErrorInit();
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
        //get current time
        //it's better to use the one call for all events occured
        gettimeofday(&cycle_time, NULL);
        dbgprint(("%x begin cycle\n", cycle_time.tv_sec * 1000 + cycle_time.tv_usec/1000));
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
                    LpxCbDns(sda);
                }
            }
            else
            {
                //non-connected socket closed the other side -> kill it here
                if ((events[i].events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR)))
                {
                    dbgprint(("hup! %d\n", sda->fd));
                    if(!LpxSdGetFlag(sda, LPX_FLAG_CONN | LPX_FLAG_WCON))
                    {
                        dbgprint(("force kill %d\n",sda->fd));
                        LpxCbKill(sda);
                        continue;
                    }
                    LpxSdSetFlag(sda, LPX_FLAG_HUP);
                }
                //don't do anything when socket is waiting for DNS
                if (LpxSdGetFlag(sda, LPX_FLAG_WAIT))
                {
                    continue;
                }
                //timeout control
                LpxSdUpdateTimestampExplicit(sda, &cycle_time);
                //data
                if (LpxSdGetFlag(sda, LPX_FLAG_WCON))
                {
                    if (events[i].events & EPOLLOUT)
                    {
                        dbgprint(("success calling %d\n", sda->fd));
                        LpxCbSuccess(sda);
                    }
                    else
                        continue;
                }
                //we lost the other connection, process it
                if (sda->other == NULL && LpxSdGetFlag(sda, LPX_FLAG_CONN))
                {
                    LpxCbKill(sda);
                    continue;
                }
                //there is something to write, and we can do that?
                if ((events[i].events & EPOLLOUT) && sda->http_out_size != 0)
                {
                    LpxCbWrite(sda);
                }
                
                //socket in the finish write state and nothing left to write?
                if (LpxSdGetFlag(sda, LPX_FLAG_FINWR) && sda->http_out_size == 0)
                {
                    LpxCbKill(sda);
                    continue;
                }
                //perform passthrough reading or http parsing
                if (LpxSdGetFlag(sda, LPX_FLAG_CONN | LPX_FLAG_HTTP) && (events[i].events & EPOLLIN))
                {
                    LpxCbRead(sda);
                }
                //socket died, kill it
                if (LpxSdGetFlag(sda, LPX_FLAG_DEAD))
                {
                    LpxCbKill(sda);
                    continue;
                }
            }
        }
        dbgprint(("end cycle\n"));
        //perform PP
        list_elem = LpxSdGlobalListPP.next;
        while(list_elem != NULL)
        {
            dbgprint(("pp called\n"));
            sda = LPX_SD_FROM_PP_LIST(list_elem);
            //prepare next list element
            LpxListRemove(list_elem);
            list_elem = LpxSdGlobalListPP.next;
            //do callbacks depending on PP task
            if(LpxSdGetFlag(sda, LPX_FLAG_PP_KILL))
            {
                LpxCbKill(sda);
                continue;
            }
            if(LpxSdGetFlag(sda, LPX_FLAG_PP_WRITE))
            {
                LpxSdClearFlag(sda, LPX_FLAG_PP_WRITE);
                LpxCbWrite(sda);
            }
            if(LpxSdGetFlag(sda, LPX_FLAG_PP_READ))
            {
                LpxSdClearFlag(sda, LPX_FLAG_PP_READ);
                LpxCbRead(sda);
            }
            dbgprint(("test2: %d %d %d\n", sda->fd, LpxSdGetFlag(sda, LPX_FLAG_FINWR), sda->http_out_size));
            if (LpxSdGetFlag(sda, LPX_FLAG_PP_KILL | LPX_FLAG_DEAD) || ((LpxSdGetFlag(sda, LPX_FLAG_FINWR) && sda->http_out_size == 0)) )
            {
                LpxCbKill(sda);
            }
        }
    }
}
