#include "lpx_net.h"

void LpxNetMakeNonbl(int sock)
{
    int flags, s;
    flags = fcntl (sock, F_GETFL, 0);
    flags |= O_NONBLOCK;
    s = fcntl (sock, F_SETFL, flags);
}