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
    dbgprint(("cb dns is called\n"));
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

void LpxParseFinishPass(SD * sda)
{
    int rest_data_size, out_free_space, required_data_pass, http_req_size, size_to_skip;
    rest_data_size = sda->http_in_size - sda->http_parse_ptr;
    out_free_space = LPX_SD_HTTP_BUF_SIZE - sda->other->http_out_size;
    http_req_size = sda->http_temp_ptr;
    required_data_pass = sda->http_limit;
    size_to_skip = required_data_pass + sda->http_parse_ptr;
    //we delayed the http parsing until this, so it must be true
    assert(http_req_size + rest_data_size <= out_free_space);
    //copy the output request
    memcpy(sda->other->http_out_data + sda->other->http_out_size, temp_buf + LPX_SD_SIZE/2, http_req_size);
    sda->other->http_out_size += http_req_size;
    if (rest_data_size > required_data_pass) //we can process pass w/o mode switching
    {
        //copy the data
        memcpy(sda->other->http_out_data + sda->other->http_out_size, sda->http_parse_ptr, required_data_pass);
        sda->other->http_out_size += required_data_pass;
        //prepare for the next part parsing
        memcpy(sda->http_in_data, sda->http_in_data + size_to_skip, size_to_skip);
        sda->http_in_size -= size_to_skip;
        assert(sda->http_in_size >= 0);
        sda->http_limit = 0;
    }
    else //we have to switch the mode
    {
        //copy the data part
        memcpy(sda->other->http_out_data + sda->other->http_out_size, sda->http_parse_ptr, rest_data_size);
        sda->other->http_out_size += rest_data_size;
        sda->http_limit -= rest_data_size;
        sda->http_in_size = 0;
        LpxSdClearFlag(LPX_FLAG_HTTP); //switch the reading mode
    }
    sda->http_parse_ptr = 0;
    sda->http_in_ptr = 0;
    sda->http_temp_ptr = 0;
    //plan the writing
    LpxPP(sda->other, LPX_FLAG_PP_WRITE);
    //plan further reading/parsing
    LpxPP(sda, LPX_FLAG_PP_READ);
}

void LpxParseFinishHttp(SD * sda)
{
    //copy the parsed request to other's output
    memcpy(sda->other->http_out_data + sda->other->http_out_size,
            temp_buf + LPX_SD_SIZE/2, sda->http_temp_ptr);
    sda->other->http_out_size += sda->http_temp_ptr;
    assert(sda->other->http_out_size <= LPX_SD_HTTP_BUF_SIZE);
    //plan the writing
    LpxPP(sda->other, LPX_FLAG_PP_WRITE);
    //cut the parsed http data
    memcpy(sda->http_in_data, sda->http_in_data + sda->http_parse_ptr, sda->http_in_size - sda->http_parse_ptr);
    sda->http_parse_ptr = 0;
    sda->http_in_ptr = 0;
    sda->http_limit = 0;
    sda->http_temp_ptr = 0;
    sda->http_in_size = sda->http_in_size - sda->http_parse_ptr;
    //plan the further HTTP parsing
    LpxPP(sda, LPX_FLAG_PP_READ);
}

void LpxParseFinishDns(SD * sda)
{
    struct gaicb * gaiptr;
    extern LpxList LpxSdGlobalListDns;
    gaiptr = &(sda->dns_gai);
    getaddrinfo_a(GAI_NOWAIT, &gaiptr, 1, &(sda->host));
    LpxSdSetFlag(sda, LPX_FLAG_WAIT);
    LpxListAddTail(&LpxSdGlobalListDns, &(sda->service));
    dbgprint(("dns finish called\n"));
    //don't do anything to http parsed data, process it later
}

void LpxCbParse(SD * sda)
{
    int read_result, parse_result;
    dbgprint(("cb parse\n"));
    if (!LpxSdGetFlag(sda, LPX_FLAG_PARSE_DEL))
    {
        read_result = LpxNetRead(sda, 0);
        if (read_result < 0)
        {
            dbgprint(("cb parse - fast fail\n"));
            LpxSdSetFlag(sda, LPX_FLAG_DEAD);
            return;
        }
        parse_result = LpxParseFastCheck(sda);
        if (parse_result <= 0) //more data required
        {
            if(read_result > 0 || parse_result < 0) //buffer is over, but header hasn't ended yet
            {
                dbgprint(("parse-fast error\n"));
                LpxFinWr(sda, &LpxErrGlobal400);
                return;
            }
            dbgprint(("parse-more data\n"));
            return; //wait for more data
        }
    }
    else
    {
        dbgprint(("parse delay unlock\n"));
        LpxSdClearFlag(sda, LPX_FLAG_PARSE_DEL);
    }
    dbgprint(("fast parse-done\n"));
    //if there is no space in output buffer where to put the http request, delay the processing
    if (sda->other != NULL && sda -> http_in_size > (LPX_SD_HTTP_BUF_SIZE - sda -> other -> http_out_size))
    {
        dbgprint(("parse delay happened\n"));
        LpxSdSetFlag(sda, LPX_FLAG_LOCK | LPX_FLAG_PARSE_DEL);
        return LpxCbWrite(sda->other);
    }
    parse_result = LpxParseMain(sda);
    if (parse_result < 0 || sda->host_size == 0) //error happened
    {
        LpxFinWr(sda, &LpxErrGlobal400);
        return;
    }
    if (parse_result == 0) //auth error
    {
        LpxFinWr(sda, &LpxErrGlobal407);
        return;
    }
    dbgprint(("parse-success\n"));
    if (LpxSdGetFlag(sda, LPX_FLAG_CONN))
    {
        if (sda->http_limit > 0)
        {
            return LpxParseFinishPass(sda);
        }
        else
        {
            return LpxParseFinishHttp(sda);
        }
    }
    else
    {
        LpxParseFinishDns(SD * sda);
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
