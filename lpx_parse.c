#include "lpx_parse.h"

//this check must be as fast as possible
int LpxParseFastCheck(SD * sda)
{
    char * http_ptr, * http_end;
    unsigned char c;
    //ensure we don't skip previous parts of \r\n\r\n sign
    http_ptr = sda->http_in_data + MAX(0, sda->http_in_ptr - 3);
    //count till the end of http buffer
    http_end = sda->http_in_data + sda->http_in_size;
    for ( ; http_ptr < http_end; ++http_ptr)
    {
        c = (unsigned char) *http_ptr;
        if ( c == '\r') //  \r catched?
        {
            if (http_ptr < http_end - 3) //check if it's the end of http header
            {
                if (memcmp(http_ptr, "\r\n\r\n", 4) == 0) //if it's really the end of header
                {
                    sda->http_in_ptr = http_ptr - sda->http_in_data + 4; //record the processed part
                    return 1; //return ok
                }
                if (http_ptr[1] == '\n') //it's not header, so it can be just simple \r\n
                    ++http_ptr; //skip '\n' for now
                else
                {
                    dbgprint(("parse - bad n\n"));
                    return -1; //it's not '\n', which is not possible, return error
                }
            }
            else
                break;
        }
        else if(c >= 0x80 || c < 0x20) //non-printable symbols can't be in http header
        {
            dbgprint(("parse - bad c\n"));
            return -1; //return error
        }
    }
    sda->http_in_ptr = http_ptr - sda->http_in_data; //save the pointer where we stopped checking
    return 0; //say more data is required
}

void LpxParseLowercase(SD * sda)
{
    int i;
    char c;
    for (i = 0; i < sda->http_in_ptr; i++)
    {
        c = sda->http_in_data[i];
        if (c >= 'A' && c <= 'Z')
            temp_buf[i] = c + ('a' - 'A');
        else
            temp_buf[i] = c;
    }
}

int LpxParseReqType(SD * sda)
{
    LpxSdClearFlag(sda, LPX_FLAG_TGET | LPX_FLAG_TPOST | LPX_FLAG_TCON);
    if (memcmp(temp_buf, "get ", 4) == 0)
    {
        LpxSdSetFlag(sda, LPX_FLAG_TGET);
        sda->http_parse_ptr = 4;
        memcpy(sda->http_temp_data, sda->http_in_data, 4);
        sda->http_temp_ptr = 4;
        return 1;
    }
    if (memcmp(temp_buf, "head ", 5) == 0)
    {
        LpxSdSetFlag(sda, LPX_FLAG_TGET);
        sda->http_parse_ptr = 5;
        memcpy(sda->http_temp_data, sda->http_in_data, 5);
        sda->http_temp_ptr = 5;
        return 1;
    }
    if (memcmp(temp_buf, "post ", 5) == 0)
    {
        LpxSdSetFlag(sda, LPX_FLAG_TPOST);
        sda->http_parse_ptr = 5;
        memcpy(sda->http_temp_data, sda->http_in_data, 5);
        sda->http_temp_ptr = 5;
        return 1;
    }
    if (memcmp(temp_buf, "connect ", 8) == 0)
    {
        LpxSdSetFlag(sda, LPX_FLAG_TCON);
        sda->http_parse_ptr = 8;
        memcpy(sda->http_temp_data, sda->http_in_data, 8);
        sda->http_temp_ptr = 8;
        return 1;
    }
    return 0;
}

int LpxParseHost(SD * sda)
{
    int port, temp;
    char * tcs = NULL, * tcp = NULL, c = 0, * host_end;
    char * input = sda->http_in_data + sda->http_parse_ptr;
    if (LpxSdGetFlag(sda, LPX_FLAG_TCON)) //set default port values
    {
        port = 443;
    }
    else
    {
        port = 80;
    }
    //try to get port values from address prefix
    if (memcmp(input, "http://", 7)==0)
    {
        sda->http_parse_ptr += 7;
        port = 80;
    }
    if (memcmp(input, "https://", 8)==0)
    {
        sda->http_parse_ptr += 8;
        port = 443;
    }
    if (memcmp(input, "ftp://", 6)==0)
    {
        sda->http_parse_ptr += 6;
        port = 22;
    }
    if (input[0] == '/') //no host address
    {
        dbgprint(("no host address\n"));
        sda->http_parse_ptr += 1;
        return 0; //host wasn't updated
    }
    tcs = input = sda->http_in_data + sda->http_parse_ptr;
    while ((c = *tcs) != '/' && c > 0x20)
    {
        if (c == ':')
            tcp = tcs;
        ++tcs;
    }
    if (c != '/' && !LpxSdGetFlag(sda, LPX_FLAG_TCON))
    {
        dbgprint(("must be slash on end\n"));
        return -1; //some error happened
    }
    if (tcp != NULL && tcp < tcs) //parse connection port
    {
        host_end = tcp;
        temp = atoi(tcp + 1);
        if (temp > 0)
            port = temp;
    }
    else
        host_end = tcs;
    sda->http_parse_ptr = tcs - sda->http_in_data;
    if (LpxSdGetFlag(sda, LPX_FLAG_CONN)) //already connected
    {
        if (memcmp(input, sda->host, sda->host_size) != 0 || port != sda->port) //the host or port is different
        {
            dbgprint(("no host match ^%s^ ^%s^ %d %d %d\n", sda->host, input, sda->host_size, port, sda->port));
            //return -1;
            //disconnect it!
            LpxSdClearFlag(sda, LPX_FLAG_CONN);
            sda->other->other = NULL;
            LpxPP(sda->other, LPX_FLAG_PP_KILL);
            sda->other = NULL;
        }
        else
            return 0; //host wasn't updated
    }
    sda->port = port;
    if (tcs - input > LPX_SD_HOST_SIZE) //host name overflow
        return -1;
    else //copy host data
    {
        sda->host_size = temp = host_end - input;
        memcpy(sda->host, input, temp);
        sda->host[temp] = 0;
        dbgprint(("host parse: port %d host ^%s^\n", sda->port, sda->host));
        sprintf(sda->service, "%d", sda->port);
    }
    return 1;
}

int LpxParseHeaders(SD * sda)
{
    char * out_data, * in_data, * in_lowercase, *tc, c, auth = 0;
    int temp;
    if (LpxGlobalPassData.buf == NULL)
        auth = 1;
    sda->http_limit = 0;
    //copy the rest of first header
    out_data = sda->http_temp_data + sda->http_temp_ptr;
    in_data = sda->http_in_data + sda->http_parse_ptr;
    do
    {
        c = *in_data;
        *out_data = c;
        ++in_data;
        ++out_data;
    }
    while( c != '\n' );
    LpxSdClearFlag(sda, LPX_FLAG_KAL);
    //parse other headers
    in_lowercase = (in_data - sda->http_in_data + temp_buf);
    while( *in_data != '\r' )
    {
        tc = strchr(in_data, '\r') + 2;
        temp = tc - in_data;
        if (memcmp(in_lowercase, "proxy-connection: keep-alive", 28) ==0) 
            LpxSdSetFlag(sda, LPX_FLAG_KAL);
        if (memcmp(in_lowercase, "proxy-authorization: basic ", 27) == 0)
        {
            if (LpxGlobalPassData.buf != NULL && memcmp(in_data + 27, LpxGlobalPassData.buf, LpxGlobalPassData.len) == 0)
                auth = 1;
        }
        else if (memcmp(in_lowercase, "proxy", 5) == 0) //skip other proxy related headers
        {
            
        }
        else //copy the whole header
        {
            //save the host, if there were no already
            if (sda->host_size == 0 && memcmp(in_lowercase, "host: ", 6) == 0)
            {
                sda->host_size = temp - 8;
                memcpy(sda->host, in_data + 6, sda->host_size);
            }
            if (memcmp(in_lowercase, "connection: keep-alive", 26) ==0) 
                LpxSdSetFlag(sda, LPX_FLAG_KAL);
            //save the content length
            if (memcmp(in_lowercase, "content-length: ", 16) == 0)
            {
                sda->http_limit = atoi(in_lowercase + 16);
            }
            memcpy(out_data, in_data, temp);
            out_data += temp;
        }
        in_data += temp;
        in_lowercase += temp;
    }
    memcpy(out_data, "\r\n", 3);
    sda->http_temp_ptr = out_data - sda->http_temp_data + 2;
    sda->http_parse_ptr = in_lowercase - temp_buf + 2;
    dbgprint(("parse out: ^%s^\n", sda->http_temp_data));
    return auth;
}

int LpxParseMain(SD * sda)
{
    int result;
    //not fully implemented yet
    dbgprint(("parse lowercase\n"));
    LpxParseLowercase(sda);
    //if can't parse request type - error
    dbgprint(("parse req type\n"));
    if (!LpxParseReqType(sda))
    {
        dbgprint(("advparse - type err\n"));
        return -1;
    }
    //connect request is allowed only as first request (change it maybe?)
    if (LpxSdGetFlags(sda, LPX_FLAG_CONN | LPX_FLAG_TCON))
    {
        dbgprint(("advparse - conn but con\n"));
        return -1;
    }
    dbgprint(("parse host\n"));
    if (LpxParseHost(sda) < 0)
        return -1;
    result = LpxParseHeaders(sda);
    if (result <= 0)
        return result;
    //content length allowed only on post request
    if (!LpxSdGetFlags(sda, LPX_FLAG_TPOST) && sda->http_limit > 0)
        return -1;
    //check for errors
    if (sda->http_in_ptr != sda->http_parse_ptr)
    {
        dbgprint(("!!inprt != parseptr!!\n"));
    }
    if (sda->host_size == 0 || sda->port == 0)
        return -1;
}
