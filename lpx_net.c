#include "lpx_net.h"

void LpxNetMakeNonbl(int sock)
{
    int flags, s;
    flags = fcntl (sock, F_GETFL, 0);
    flags |= O_NONBLOCK;
    s = fcntl (sock, F_SETFL, flags);
}

int LpxNetWrite(SD * sda)
{
    int to_write, temp;
    
    to_write = sda->http_out_size - sda->http_out_ptr;
    if (to_write == 0)
    {
        sda->http_out_size = sda->http_out_ptr = 0;
        return 1;
    }
    temp = send(sda->fd, sda->http_out_data + sda->http_out_ptr, to_write, MSG_NOSIGNAL);
    if (temp == 0) //socket closed
        return -1;
    if (temp < 0 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) //socket busy
        return 0;
    if (temp < 0) //other error happened
        return -1;
    if (temp < to_write) //not all data has been written
    {
        sda->http_out_ptr += temp; //update pointer
        if (sda->http_out_ptr >= LPX_SD_HTTP_BUF_SIZE / 2) //relocate the data to make space for reading
        {
            memcpy(sda->http_out_data, sda->http_out_data + sda->http_out_ptr, sda->http_out_size - sda->http_out_ptr);
            sda->http_out_size -= sda->http_out_ptr;
            sda->http_out_ptr = 0;
        }
        return 0; //some data left
    }
    //everything has been written w/o errors
    sda->http_out_size = sda->http_out_ptr = 0;
    return 1;
}

int LpxNetRead(SD * sda, int type)
{
    LpxBuf * buffer;
    int to_read, temp, fd, limit = -1;
    if (type == 0) //HTTP reading
    {
        buffer = &(sda->http_in_buf);
        to_read = LPX_SD_IN_SIZE - buffer->size;
    }
    else
    {
        if (sda->other == NULL)
            return -1;
        buffer = &(sda->other->http_out_buf);
        to_read = LPX_SD_HTTP_BUF_SIZE - buffer->size;
        limit = sda->other->http_limit;
        if (limit >= 0)
        {
            to_read = MIN(to_read, limit);
        }
    }
    if (to_read == 0)
        return 1; //wasn't able to read anything, socket not blocked
    temp = recv(sda->fd, buffer->data + buffer->size, to_read, MSG_NOSIGNAL);
    if (temp == 0) //socket closed
        return -1; //return error
    if (temp < 0 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) //socket busy
        return 0; //no data has been read
    if (temp < 0) //other error happened
        return -1;
    //update the limit
    if (limit > 0)
        limit -= temp;
    if (temp < to_read)
    {
        buffer->size += temp;
        return 0; //socket got blocked
    }
    buffer->size += temp;
    return 1; //socket hasn't been blocked
}
