#include "lpx_cb.h"

void LpxCbKill(SD * sda)
{
    dbgprint(("cb kill\n"));
    LpxSdDestroy(sda);
}

void LpxCbAccept(SD * sda)
{
    int accept_fd, temp;
    SD * new_sd;
    struct sockaddr_in temp_adr;
    dbgprint(("cb accept\n"));
    while(1)
    {
        temp = sizeof(struct sockaddr_in);
        accept_fd = accept (sda->fd, (struct sockaddr * )&temp_adr, &temp);
        if (accept_fd < 0)
            break;
        new_sd = LpxSdCreate();
        memcpy(&new_sd->src_adr, &temp_adr, temp);
        if (getsockname(accept_fd, (struct sockaddr * )(&(new_sd->dst_adr)), &temp) != 0)
        {
            dbgprint(("getname fail"));
        }
        LpxNetMakeNonbl(accept_fd);
        LpxSdInit(new_sd, accept_fd, LPX_FLAG_OPEN | LPX_FLAG_CLIENT | LPX_FLAG_HTTP, 
                sda->efd, EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP);
        dbgprint(("socket %d registered\n", accept_fd));
    }
}

void LpxCbWrite(SD * sda)
{
    int res;
    extern LpxList LpxSdGlobalListPP;
    SD * other;
    res = LpxNetWrite(sda);
    if (res == 1 && LpxSdGetFlag(LPX_FLAG_LOCK)) //unlock required
    {
        //post-process the opposite side for reading
        other = sda->other;
        assert(other != NULL);
        LpxSdSetFlag(other, LPX_FLAG_PP | LPX_FLAG_PP_READ);
        LpxListAddTail(&LpxSdGlobalListPP, &(other->pp_list));
    }
}