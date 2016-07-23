#include "lpx_cb.h"

void LpxCbKill(SD * sda)
{
    dbgprint(("cb kill\n"));
    if (sda->other != NULL)
    {
        if (LpxSdGetFlag(sda->other, LPX_FLAG_SERVER) || !LpxSdGetFlags(sda->other, LPX_FLAG_HTTP | LPX_FLAG_KAL))
            LpxFinWr(sda->other, NULL);
        else
        {
            LpxSdClearFlag(sda->other, LPX_FLAG_CONN);
        }
    }
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
    dbgprint(("fin wr %d\n", sda->fd));
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

void LpxCbDns(SD * sd_dns)
{
    extern LpxList LpxSdGlobalListDns;
    struct signalfd_siginfo fdsi;
    int temp;
    SD * sda;
    LpxList * list_elem;
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
        LpxListRemove(list_elem);
        list_elem = LpxSdGlobalListDns.next;
        //check if the request is done
        temp = gai_error(&(sda->dns_gai));
        if (temp != EAI_INPROGRESS)
        {
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
        
    }
}

void LpxSayEstablish(SD * sda)
{
    dbgprint(("what does the fox say?\n"));
    memcpy(sda->http_out_data + sda->http_out_size, LpxEstGlobal.buf, LpxEstGlobal.len);
    sda->http_out_size += LpxEstGlobal.len;
    LpxPP(sda, LPX_FLAG_PP_WRITE);
}

void LpxCbSuccess(SD * sda)
{
    //if no other side - kill
    if (sda->other == NULL)
    {
        LpxSdSetFlag(sda, LPX_FLAG_DEAD);
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
    //not implemented yet
    tfd = socket(AF_INET, SOCK_STREAM , 0);
    if (tfd < 0)
    {
        dbgprint(("can't create new socket\n"));
        return LpxFinWr(sda, &LpxErrGlobal500);
    }
    new_sd = LpxSdCreate();
    //perform connection
    adrin = & (sda->dst_adr);
    adrin -> sin_port = 0; //any port
    temp = bind(tfd, (struct sockaddr*)adrin, sizeof(struct sockaddr_in));
    if (temp != 0)
    {
        dbgprint(("can't bind to IP\n"));
        return LpxFinWr(sda, &LpxErrGlobal500);
    }
    temp = connect(tfd, (struct sockaddr *) & (sda->trg_adr), sizeof(struct sockaddr_in));
    if (temp != 0 && temp != EINPROGRESS)
    {
        dbgprint(("connect fail\n"));
        return LpxFinWr(sda, &LpxErrGlobal500);
    }
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
