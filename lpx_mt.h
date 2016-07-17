//mainternance stuff.
//I also put here everything small and uncategorised

#pragma once
#include "includes.h"
#include "lpx_sd.h"
#include "lpx_mem.h"

//simple structure describing a constant string
typedef struct LpxConstString
{
    char * buf;
    int len;
} LpxConstString;

void LpxConstStringInit(LpxConstString * str, const char * in_str);
void LpxConstStringFree(LpxConstString * str);

//global error messages
extern LpxConstString LpxErrGlobal400; //bad request
extern LpxConstString LpxErrGlobal407; //auth required
extern LpxConstString LpxErrGlobal503; //service unavailable
extern LpxConstString LpxErrGlobal504; //gateway timeout