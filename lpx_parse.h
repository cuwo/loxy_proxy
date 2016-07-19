#pragma once

#include "includes.h"
#include "lpx_sd.h"

//check if the http header has received completely
//also checks for bad symbols
//returns 0 in case of fail, -1 in case of no header end
//returns http header end index in case of success
int LpxParseFastCheck(SD * sda);

int LpxParseMain(SD * sda);