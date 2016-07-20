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
        sda->http_parse_ptr += 4;
        return 1;
    }
    if (memcmp(temp_buf, "post ", 5) == 0)
    {
        LpxSdSetFlag(sda, LPX_FLAG_TPOST);
        sda->http_parse_ptr += 5;
        return 1;
    }
    if (memcmp(temp_buf, "connect ", 8) == 0)
    {
        LpxSdSetFlag(sda, LPX_FLAG_TCON);
        sda->http_parse_ptr += 8;
        return 1;
    }
    return 0;
}

int LpxParseHost(SD * sda)
{
    
}

int LpxParseMain(SD * sda)
{
    //not fully implemented yet
    sda->http_parse_ptr = 0;
    LpxParseLowercase(sda);
    //if can't parse request type - error
    if (!LpxParseReqType(sda))
    {
        dbgprint(("advparse - type err\n"));
        return -1;
    }
    //connect request is allowed only as first request (change it maybe?)
    if (LpxSdGetFlags(LPX_FLAG_CON | LPX_FLAG_TCON))
    {
        dbgprint(("advparse - conn but con\n"));
        return -1;
    }
    //don't allow post as first request (change it later?)
    if (sd->other == NULL && LpxSdGetFlag(LPX_FLAG_TPOST))
    {
        dbgprint(("advparse - post on conn\n"));
        return -1;
    }
    
    return -1;
}
