#include "lpx_mt.h"

void LpxConstStringInit(LpxConstString * str, const char * in_str)
{
    int str_len = strlen(in_str);
    str -> buf = LpxMemSafeAlloc(str_len + 1);
    memcpy(str -> buf, in_str, str_len);
    str -> len = str_len;
}

void LpxConstStringFree(LpxConstString * str)
{
    LpxMemSafeFree(str -> buf);
}

LpxConstString LpxEstGlobal;
LpxConstString LpxErrGlobal400;
LpxConstString LpxErrGlobal407;
LpxConstString LpxErrGlobal500;
LpxConstString LpxErrGlobal503;
LpxConstString LpxErrGlobal504;
