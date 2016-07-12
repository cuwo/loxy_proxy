#include "lpx_sd.h"

//global list of all created SDs
LpxList LpxSdGlobalListMain = {NULL, NULL};
//global list of SDs with pending DNS requests
LpxList LpxSdGlobalListDns = {NULL, NULL};
//global list of SDs requiring post-processing
LpxList LpxSdGlobalListPost = {NULL, NULL};

SD * LpxSdCreate()
{
    SD * new_sd;
    //assume the memory pool block is set to LPX_SD_SIZE
    new_sd = (SD*)LpxMemPoolAlloc();
    if (new_sd == NULL)
        return NULL;
    memset(new_sd, 0, LPX_SD_CLEAN); //clear everything except buffers
    LpxListAddTail(&LpxSdGlobalListMain, &(new_sd->sd_list));
    return new_sd;
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
    LpxSdClose(sda); //close connections (if not done yet)
    LpxMemPoolFree(sda); //free the allocated pool
}

void LpxSdDestroyAll()
{
    SD * sda = LpxSdGlobalListMain.next;
    while(sda != NULL)
    {
        LpxSdDestroy(sda);
        sda = LpxSdGlobalListMain.next;
    }
}

void LpxSdUpdateTimestamp(SD * sda)
{
    struct timeval last_ts = sda->ts;
    gettimeofday(&(sda->ts), NULL);
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

inline void LpxSdClearFlag(SD * sda, unsigned int flag)
{
    sda->flags &= ~flag;
}