#pragma once
#include "includes.h"
#include "lpx_list.h"
//memory-related things

//when Empty/Alloc > alloc_ratio, memfree happens
//code must leave at least Alloc*free_ratio blocks
#define LPX_POOL_ALLOC_RATIO 2
#define LPX_POOL_FREE_RATIO 2

#define LPX_ALLOC_SIGN1 0xDEADC0BA
#define LPX_ALLOC_SIGN2 0xC001B0DA
#define LPX_POOL_SIGN 0x9EBAC0F3

//init the memory pool
int LpxMemPoolInit(int block_size);

//destroy and free the whole memory pool
void LpxMemPoolDestroy();

//check the ratio and free some pool blocks
void LpxMemPoolFlush();

//custom data allocation
void * LpxMemSafeAlloc(int size);

//custom data free
void LpxMemSafeFree(void * adr);

//pool memory block allocation
void * LpxMemPoolAlloc();

//pool memory block free (put back into pool)
void LpxMemPoolFree(void * adr);

//pool memory block free (real free)
void LpxMemPoolTrueFree(void * adr);

//allocated buffer overflow check
int LpxMemCheckOverflow(void * adr);
