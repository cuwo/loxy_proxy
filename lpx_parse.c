#include "lpx_parse.h"

//this check must be as fast as possible
int LpxParseFastCheck(SD * sda)
{
    char * http_ptr, * http_end;
    unsigned char c;
    //ensure we don't skip previous parts of \r\n\r\n sign
    http_ptr = MAX(0, sda->http_in_data + sda->http_in_ptr - 3);
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
                    return http_ptr - sda->http_in_data + 4; //return the result size of the header
                }
                if (http_ptr[1] == '\n') //it's not header, so it can be just simple \r\n
                    ++http_ptr; //skip '\n' for now
                else
                    return 0; //it's not '\n', which is not possible, return error
            }
            else
                break;
        }
        else if(c >= 0x80 || c < 0x20) //non-printable symbols can't be in http header
            return 0; //return error
    }
    sda->http_in_ptr = http_ptr - sda->http_in_data; //save the pointer where we stopped checking
    return -1; //say more data is required
}
