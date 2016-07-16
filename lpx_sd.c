#include "lpx_sd.h"

//global list of all created SDs
LpxList LpxSdGlobalListMain = {NULL, NULL};
//global list of SDs with pending DNS requests
LpxList LpxSdGlobalListDns = {NULL, NULL};
//global list of SDs requiring post-processing
LpxList LpxSdGlobalListPP = {NULL, NULL};

SD * LpxSdCreate()
{
    SD * new_sd;
    //assume the memory pool block is set to LPX_SD_SIZE
    new_sd = (SD*)LpxMemPoolAlloc();
    if (new_sd == NULL)
        return NULL;
    LpxListAddTail(&LpxSdGlobalListMain, &(new_sd->sd_list));
    LpxSdUpdateTimestamp(new_sd);
    return new_sd;
}

void LpxSdInit(SD * sda, int socket, unsigned int flags, int epoll_fd, unsigned int epoll_flags)
{
    struct epoll_event event;
    memset(sda, 0, LPX_SD_CLEAN); //clear everything except buffers
    sda->fd = socket;
    sda->efd = epoll_fd;
    sda->flags = flags;
    event.data.ptr = sda;
    event.events = epoll_flags;
    epoll_ctl (epoll_fd, EPOLL_CTL_ADD, socket, &event);
}

void LpxSdClose(SD * sda)
{
    if (sda->fd > 0) //close socket
    {
        close(sda->fd);
        sda->fd = -1;
    }
    if (sda->other != NULL) //remove link with the other side
    {
        sda->other->other = NULL;
        sda->other = NULL;
    }
    LpxSdClearFlag(sda, LPX_FLAG_OPEN);
}

void LpxSdDestroy(SD * sda)
{
    LpxListRemove(&(sda->sd_list)); //remove from the main SD list
    LpxListRemove(&(sda->dns_list)); //remove it from other lists
    LpxListRemove(&(sda->pp_list));
    LpxSdClose(sda); //close connections (if not done yet)
    LpxMemPoolFree(sda); //free the allocated pool
}

void LpxSdDestroyAll()
{
    LpxList * list = LpxSdGlobalListMain.next;
    SD * sda;
    while(list != NULL)
    {
        sda = LPX_SD_FROM_MAIN_LIST(list);
        LpxSdDestroy(sda);
    }
}

int LpxSdUpdateTimestamp(SD * sda)
{
    struct timeval last_ts = sda->ts;
    gettimeofday(&(sda->ts), NULL);
    return (sda->ts.tv_sec - last_ts.tv_sec)*1000.0 + 
        (sda->ts.tv_usec - last_ts.tv_usec)/1000.0 + 0.5;
}

int LpxSdUpdateTimestampExplicit(SD * sda, struct timeval * tv)
{
    struct timeval last_ts = sda->ts;
    sda->ts = *tv;
    return (sda->ts.tv_sec - last_ts.tv_sec)*1000.0 + 
        (sda->ts.tv_usec - last_ts.tv_usec)/1000.0 + 0.5;
}

inline void LpxSdSetFlag(SD * sda, unsigned int flag)
{
    sda->flags |= flag;
}

inline int LpxSdGetFlag(SD * sda, unsigned int flag)
{
    return (sda->flags & flag) == 0 ? 0 : 1;
}

inline int LpxSdGetFlags(SD * sda, unsigned int flag)
{
    return (sda->flags & flag) == flag ? 1 : 0;
}

inline void LpxSdClearFlag(SD * sda, unsigned int flag)
{
    sda->flags &= ~flag;
}
