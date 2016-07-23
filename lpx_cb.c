#include "lpx_cb.h"

void LpxCbKill(SD * sda)
{
    dbgprint(("cb kill\n"));
    LpxSdDestroy(sda);
}

void LpxPP(SD * sda, unsigned int flags)
{
    extern LpxList LpxSdGlobalListPP;
    if (sda == NULL)
        return;
    LpxSdSetFlag(sda, LPX_FLAG_PP | flags);
    LpxListAddTail(&LpxSdGlobalListPP, &(sda->pp_list));
}

void LpxFinWr(SD * sda, LpxConstString * err)
{
    int temp;
    dbgprint(("fin wr\n"));
    LpxSdSetFlag(sda, LPX_FLAG_FINWR);
    if (err != NULL)
    {
        //append the data to the end of write buf
        temp =  err->len;
        //if there is enough space
        if(temp <= (LPX_SD_HTTP_BUF_SIZE - sda->http_out_size))
        {
            dbgprint(("adding %d bytes already %d full %d\n",temp, sda->http_out_size,LPX_SD_HTTP_BUF_SIZE));
            memcpy(sda->http_out_data + sda->http_out_size, err->buf, temp);
            sda->http_out_size += temp;
        }
    }
    LpxPP(sda, LPX_FLAG_PP_WRITE);
}

void LpxCbDns(SD * sda)
{
    extern LpxList LpxSdGlobalListDns;
    LpxList * list_elem;
    dbgprint(("cb dns is called\n"));
    //read the socket to block it
    
    list_elem = LpxSdGlobalListPP.next;
    while(list_elem != NULL)
}

void LpxConnect(SD * sda)
{
    dbgprint(("cb connect is called\n"));
    //perform connection 
    
    
    //finish the http parsing
    if (sda->http_limit > 0)
    {
        return LpxParseFinishPass(sda);
    }
    else
    {
        return LpxParseFinishHttp(sda);
    }
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
    dbgprint(("cb write\n"));
    SD * other;
    res = LpxNetWrite(sda);
    if (res > 0 && LpxSdGetFlag(sda, LPX_FLAG_LOCK)) //unlock required
    {
        //post-process the opposite side for reading
        other = sda->other;
        assert(other != NULL);
        LpxPP(other, LPX_FLAG_PP_READ);
        LpxSdClearFlag(sda, LPX_FLAG_LOCK);
    }
    else if (res < 0)
    {
        return LpxCbRead(sda);
    }
}

//main callback (and the most complicated one)
void LpxCbRead(SD * sda)
{
    int read_result, write_result;
    if (LpxSdGetFlag(sda, LPX_FLAG_HTTP))
        return LpxCbParse(sda);
    dbgprint(("cb read\n"));
    do
    {
        read_result = LpxNetRead(sda, 1); //read from current socket
        write_result = LpxNetWrite(sda->other); //write to the other one
    }
    while(read_result == 1 && write_result == 1); //pass the traffic until block or fail
    
    if (read_result < 0) //read failed. close this side and finish the other.
    {
        LpxSdSetFlag(sda, LPX_FLAG_DEAD);
        LpxFinWr(sda->other, NULL);
        return;
    }
    if (write_result < 0) //write failed. plan the reading from that side.
    {
        LpxPP(sda->other, LPX_FLAG_PP_READ);
        return;
    }
    if (sda -> http_limit == 0) //pass limit expired, make the socket back to HTTP and do parse
    {
        LpxSdSetFlag(sda, LPX_FLAG_HTTP);
        return LpxCbParse(sda);
    }
    if (read_result == 1) //unlock required later
    {
        LpxSdSetFlag(sda->other, LPX_FLAG_LOCK);
    }
}
