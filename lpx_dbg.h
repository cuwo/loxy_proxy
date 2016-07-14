//debug functions I will use for this project
#pragma once
#include "lpx_sd.h"

#ifdef DEBUG
#define dbgprint(a) printf a
#else
#define dbgprint(a) 
#endif

void LpxDbgPrint(const char * string, SD * sda, unsigned int arg);

void LpxDbgHex(void * data, int size);