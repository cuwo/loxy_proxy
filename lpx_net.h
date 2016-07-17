#pragma once
#include "includes.h"
#include "lpx_sd.h"

//makes socket non-blocking
void LpxNetMakeNonbl(int sock);

//writes data into socket.
//returns 1 when all the data has been written
//returns 0 when some data is still pending
//returns -1 in case of socket error
int LpxNetWrite(SD * sda);