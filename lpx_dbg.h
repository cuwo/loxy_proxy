//debug functions I will use for this project
#pragma once
#include "includes.h"

extern int dbg_mode;

#define dbgprint(a) {if (dbg_mode) printf a;}

void dbghex(void * data, int size);
