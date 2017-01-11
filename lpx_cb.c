#include "lpx_cb.h"

void LpxCbTimer(SD * sda_tio)
{
    extern LpxList LpxSdGlobalListMain;
    LpxList * list_elem;
    SD * sda;
    int tio;
    dbgprint(("cb timer!\n"));
    //read the timer to block it back
    read(sda_tio->fd, temp_buf, LPX_SD_SIZE);
    //check all the SDs for timeout
    list_elem = LpxSdGlobalListMain.next;
    while (list_elem != NULL)
    {
        sda = LPX_SD_FROM_MAIN_LIST(list_elem);
        list_elem = list_elem->next;
        if (LpxSdGetFlag(sda, LPX_FLAG_MAINT))
        {
            continue; //don't kill mainternance SDs
        }
        tio = LpxSdGetTimeout(sda, sda_tio);
        if (tio > LPX_WAIT_TIMEOUT && LpxSdGetFlag(sda, LPX_FLAG_WAIT))
        {
            LpxFinWr(sda, &LpxErrGlobal504);
        }
        else if (tio > LPX_TIMEOUT || tio > LPX_AUTH_TIMEOUT && LpxGlobalPassData.buf != NULL && !LpxSdGetFlag(sda, LPX_FLAG_AUTH))
        {
            LpxPP(sda, LPX_FLAG_PP_KILL);
        }
    }
}

void LpxCbKill(SD * sda)
{
    dbgprint(("cb kill %d %X\n", sda->fd, sda->flags));
    if (sda->other != NULL)
    {
        if (LpxSdGetFlag(sda->other, LPX_FLAG_SERVER) || !LpxSdGetFlags(sda->other, LPX_FLAG_HTTP | LPX_FLAG_KAL))
        {
            LpxFinWr(sda->other, NULL);
        }
        else
        {
            LpxSdClearFlag(sda->other, LPX_FLAG_CONN);
        }
    }
    if (sda->dns_list.prev != NULL) //dns in progress
    {
        if (gai_cancel(&(sda->dns_gai)) == EAI_CANCELED)
            LpxSdDestroy(sda);
        else
            LpxSdSetFlag(sda, LPX_FLAG_DK);
    }
    else
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

void LpxSay(SD * sda, LpxConstString * err)
{
    int temp;
    if (sda == NULL)
        return;
    dbgprint(("say %d\n", sda->fd));
    if (err != NULL)
    {
        temp = err->len;
        if(temp <= (LPX_SD_HTTP_BUF_SIZE - sda->http_out_size))
        {
            dbgprint(("adding %d bytes already %d full %d\n",temp, sda->http_out_size,LPX_SD_HTTP_BUF_SIZE));
            memcpy(sda->http_out_data + sda->http_out_size, err->buf, temp);
            sda->http_out_size += temp;
        }
    }
    LpxPP(sda, LPX_FLAG_PP_WRITE);
}

void LpxFinWr(SD * sda, LpxConstString * err)
{
    int temp;
    if (sda == NULL)
        return;
    dbgprint(("fin wr %d\n", sda->fd));
    LpxSdSetFlag(sda, LPX_FLAG_FINWR);
    LpxSay(sda, err);
}

void LpxCbDns(SD * sd_dns)
{
    extern LpxList LpxSdGlobalListDns;
    struct signalfd_siginfo fdsi;
    int temp;
    SD * sda;
    LpxList * list_elem, * list_next;
    dbgprint(("cb dns is called\n"));
    //read the socket to block it
    do
    {
        temp = read(sd_dns->fd, &fdsi, sizeof(struct signalfd_siginfo));
    }
    while (temp > 0);
    //process all requests
    list_elem = LpxSdGlobalListDns.next;
    while(list_elem != NULL)
    {
        sda = LPX_SD_FROM_DNS_LIST(list_elem);
        //get next elem
        list_next = list_elem->next;
        //check if the request is done
        temp = gai_error(&(sda->dns_gai));
        if (temp != EAI_INPROGRESS)
        {
            LpxListRemove(list_elem);
            if (sda->dns_gai.ar_result == NULL) //not resolved? say error
            {
                LpxSdClearFlag(sda, LPX_FLAG_WAIT);
                LpxFinWr(sda, &LpxErrGlobal503);
            }
            else
            {
                memcpy(&(sda->trg_adr), sda->dns_gai.ar_result->ai_addr, sizeof(struct sockaddr_in));
                freeaddrinfo(sda->dns_gai.ar_result);
                LpxSdSetFlag(sda, LPX_FLAG_DNS);
                LpxCbConnect(sda);
            }
        }
        list_elem = list_next;
    }
}

void LpxSayEstablish(SD * sda)
{
    assert(LPX_SD_HTTP_BUF_SIZE - sda->http_out_size >= LpxEstGlobal.len);
    dbgprint(("what does the fox say?\n"));
    memcpy(sda->http_out_data + sda->http_out_size, LpxEstGlobal.buf, LpxEstGlobal.len);
    sda->http_out_size += LpxEstGlobal.len;
    LpxPP(sda, LPX_FLAG_PP_WRITE);
}

void LpxCbSuccess(SD * sda)
{
    //if no other side - kill
    if (sda->other == NULL || LpxSdGetFlag(sda->other, LPX_FLAG_DEAD | LPX_FLAG_HUP))
    {
        LpxFinWr(sda, &LpxErrGlobal503);
        return;
    }
    //make both sides finally connected
    LpxSdSetFlag(sda, LPX_FLAG_CONN);
    LpxSdSetFlag(sda->other, LPX_FLAG_CONN);
    //say to the other side, it's ready
    if (LpxSdGetFlag(sda->other, LPX_FLAG_TCON))
    {
        LpxSayEstablish(sda->other);
    }
    //clear the waiting flags
    LpxSdClearFlag(sda, LPX_FLAG_WCON);
    LpxSdClearFlag(sda->other, LPX_FLAG_WAIT);
    dbgprint(("connect success %d %d\n", sda->fd, sda->other->fd));
    //finish the http parsing
    return LpxParseFinish(sda->other);
}

void LpxCbConnect(SD * sda)
{
    SD * new_sd;
    int temp, tfd;
    struct sockaddr_in * adrin;
    dbgprint(("cb connect is called\n"));
    if (LpxSdGetFlag(sda, LPX_FLAG_DK))
        return LpxCbKill(sda);
    tfd = socket(AF_INET, SOCK_STREAM , 0);
    if (tfd < 0)
    {
        dbgprint(("can't create new socket\n"));
        return LpxFinWr(sda, &LpxErrGlobal500);
    }
    //perform connection
    adrin = & (sda->dst_adr);
    adrin -> sin_port = 0; //any port
    temp = bind(tfd, (struct sockaddr*)adrin, sizeof(struct sockaddr_in));
    if (temp != 0)
    {
        close(tfd);
        dbgprint(("can't bind to IP\n"));
        return LpxFinWr(sda, &LpxErrGlobal500);
    }
    LpxNetMakeNonbl(tfd);
    temp = connect(tfd, (struct sockaddr *) & (sda->trg_adr), sizeof(struct sockaddr_in));
    if (temp != 0 && errno != EINPROGRESS)
    {
        close(tfd);
        dbgprint(("connect fail\n"));
        return LpxFinWr(sda, &LpxErrGlobal500);
    }
    new_sd = LpxSdCreate();
    LpxSdInit(new_sd, tfd, LPX_FLAG_OPEN | LPX_FLAG_SERVER | LPX_FLAG_WCON, 
                sda->efd, EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP);
    new_sd->http_limit = -1;
    //link both sides
    sda->other = new_sd;
    new_sd->other = sda;
    dbgprint(("connect to %d\n", tfd));
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
    res = LpxNetWrite(sda);
    if (res > 0 && LpxSdGetFlag(sda, LPX_FLAG_LOCK)) //unlock required
    {
        //post-process the opposite side for reading
        LpxPP(sda->other, LPX_FLAG_PP_READ);
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
    if (sda->other == NULL)
    {
        LpxSdSetFlag(sda, LPX_FLAG_DEAD);
        return;
    }
    do
    {
        read_result = LpxNetRead(sda, 1); //read from current socket
        write_result = LpxNetWrite(sda->other); //write to the other one
    }
    while(read_result == 1 && write_result == 1 && sda->http_limit != 0); //pass the traffic until block or fail
    
    if (read_result < 0) //read failed. close this side and finish the other.
    {
        LpxSdSetFlag(sda, LPX_FLAG_DEAD);
        dbgprint(("read fail %d %d\n", sda->fd, LpxSdGetFlag(sda, LPX_FLAG_DEAD)));
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
        dbgprint(("-------------->LOCK %d<-------------\n", sda->fd));
        LpxSdSetFlag(sda->other, LPX_FLAG_LOCK);
    }
}
