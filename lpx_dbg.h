//debug functions I will use for this project
#pragma once
#include "includes.h"

#ifdef DEBUG
#define dbgprint(a) printf a
#else
#define dbgprint(a) 
#endif

void dbghex(void * data, int size);
