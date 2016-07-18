#include "lpx_cb.h"

void LpxCbKill(SD * sda)
{
    dbgprint(("cb kill\n"));
    LpxSdDestroy(sda);
}

void LpxPP(SD * sda, unsigned int flags)
{
    LpxSdSetFlag(sda, LPX_FLAG_PP | flags);
    LpxListAddTail(&LpxSdGlobalListPP, &(sda->pp_list));
}

void LpxFinWr(SD * sda, LpxConstString * err)
{
    int temp;
    LpxSdSetFlag(sda, LPX_FLAG_FINWR);
    if (err != NULL)
    {
        //append the data to the end of write buf
        temp =  err->len;
        //if there is enough space
        if(temp <= (LPX_SD_HTTP_BUF_SIZE - sda->http_out_size))
        {
            memcpy(sda->http_out_data + sda->http_out_size, err->buf, temp);
            sda->http_out_size += temp;
        }
    }
    LpxPP(sda, LPX_FLAG_PP_WRITE);
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
        //kill the socket in the PP way
        LpxPP(other, LPX_FLAG_PP_KILL);
    }
}

void LpxCbParse(SD * sda)
{
    int read_result, parse_result;
    read_result = LpxNetRead(sda, 0);
    if (read_result < 0)
    {
        LpxPP(other, LPX_FLAG_PP_KILL);
        return;
    }
    parse_result = LpxParseFastCheck(sda);
    if (parse_result == 0) //more data required
    {
        if(read_result > 0) //buffer is over, but header hasn't ended yet
        {
            LpxFinWr(sda, &LpxErrGlobal400);
            return;
        }
        return; //wait for more data
    }
    parse_result = LpxParseMain(sda);
    if (parse_result < 0) //error happened
    {
        LpxFinWr(sda, &LpxErrGlobal400);
        return;
    }
    if (parse_result == 0) //auth error
    {
        LpxFinWr(sda, &LpxErrGlobal407);
        return;
    }
    LpxDnsGo(sda); //make new connection
}