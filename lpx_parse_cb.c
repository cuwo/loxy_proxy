#include "lpx_parse_cb.h"

void LpxParseClean(SD * sda)
{
    int rest_data_size;
    int temp;
    //check the limit and copy the data
    rest_data_size = sda->http_in_size - sda->http_parse_ptr;
    if (sda->http_limit > 0)
    {
        temp = MIN(sda->http_limit, rest_data_size);
        memcpy(sda->http_temp_data + sda->http_temp_ptr, sda->http_in_data + sda->http_parse_ptr, temp);
        sda->http_temp_ptr += temp;
        sda->http_parse_ptr += temp;
        sda->http_limit -= temp;
        rest_data_size -= temp;
    }
    memcpy(sda->http_in_data, sda->http_in_data + sda->http_parse_ptr, rest_data_size);
    sda->http_in_ptr = 0;
    sda->http_in_size -= sda->http_parse_ptr;
    dbgprint(("http clean done. size in: %d size out: %d\n", sda->http_in_size, sda->http_temp_ptr));
}

void LpxParseFinish(SD * sda)
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
    memcpy(sda->other->http_out_data + sda->other->http_out_size, sda->temp_data, http_req_size);
    sda->other->http_out_size += http_req_size;
    if (rest_data_size > required_data_pass) //we can process pass w/o mode switching
    {
        //copy the data
        memcpy(sda->other->http_out_data + sda->other->http_out_size, sda->http_in_data+sda->http_parse_ptr, required_data_pass);
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
        memcpy(sda->other->http_out_data + sda->other->http_out_size, sda->http_in_data+sda->http_parse_ptr, rest_data_size);
        sda->other->http_out_size += rest_data_size;
        sda->http_limit -= rest_data_size;
        sda->http_in_size = 0;
        LpxSdClearFlag(sda, LPX_FLAG_HTTP); //switch the reading mode
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
            sda->temp_data, sda->http_temp_ptr);
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
    int result;
    //try to parse host as IP
    result = inet_pton(AF_INET, cba->host, &(sda->trg_adr.sin_addr));
    if (temp == 1)
    {
        return LpxCbConnect(sda);
    }
    //fail? then call DNS
    gaiptr = &(sda->dns_gai);
    gaiptr->ar_name = sda->host;
    gaiptr->ar_service = sda->service;
    gaiptr->ar_request = NULL;
    gaiptr->ar_result = NULL;
    getaddrinfo_a(GAI_NOWAIT, &gaiptr, 1, &(sda->dns_sev));
    LpxSdSetFlag(sda, LPX_FLAG_WAIT);
    LpxListAddTail(&LpxSdGlobalListDns, &(sda->dns_list));
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
        if (parse_result <= 0) //more data required, or error happened
        {
            //buffer is over, but header hasn't ended yet
            //or the check has failed
            if(read_result > 0 || parse_result < 0)
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
    if (sda->other != NULL && LPX_SD_TEMP_SIZE > (LPX_SD_HTTP_BUF_SIZE - sda -> other -> http_out_size))
    {
        dbgprint(("parse delay happened\n"));
        LpxSdSetFlag(sda, LPX_FLAG_PARSE_DEL);
        LpxSdSetFlag(sda->other, LPX_FLAG_LOCK); //unlock required after writing
        LpxPP(sda->other, LPX_FLAG_PP_WRITE); //postprocess writing
        return;
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
    LpxParseClean(sda);
    if (LpxSdGetFlag(sda, LPX_FLAG_CONN))
    {
        return LpxParseFinish(sda);
    }
    else
    {
        LpxParseFinishDns(sda);
    }
}