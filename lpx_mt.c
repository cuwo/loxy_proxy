#include "lpx_mt.h"

void LpxConstStringInit(LpxConstString * str, const char * in_str)
{
    int str_len = strlen(str);
    str -> buf = LpxMemSafeAlloc(str_len + 1);
    memcpy(str -> buf, str, str_len);
}

void LpxConstStringFree(LpxConstString * str)
{
    LpxMemSafeFree(str -> buf);
}