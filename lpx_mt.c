#include "lpx_mt.h"

void LpxConstStringInit(LpxConstString * str, const char * in_str)
{
    int str_len = strlen(in_str);
    str -> buf = LpxMemSafeAlloc(str_len + 1);
    memcpy(str -> buf, in_str, str_len);
}

void LpxConstStringFree(LpxConstString * str)
{
    LpxMemSafeFree(str -> buf);
}
