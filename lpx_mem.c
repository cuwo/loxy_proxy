#include "lpx_mem.h"

//is the pool inited
int LpxMemGlobalPoolInit = 0;

//size of each pool's data block
int LpxMemGlobalPoolBlockSize;

//list of empty blocks for allocation
LpxList LpxMemGlobalPoolFreeList;

//list of blocks in use
LpxList LpxMemGlobalPoolAllocList;

//how much elements are allocated
int LpxMemGlobalPoolAllocCount;

//how much elements are available
int LpxMemGlobalPoolEmptyCount;

//to detect memory leaks, etc
int LpxMemGlobalSafeAllocCount = 0;
int LpxMemGlobalSafeFreeCount = 0;

void * LpxMemSafeAlloc(int size)
{
    void * data;
    void * actual_data;
    int new_size;
    if (size <= 0)
        return NULL;
    new_size = ((size + 3) & 0xFFFFFFFC);
    data = malloc(new_size + 0xC);
    if (data == NULL)
        return NULL;
    ((int*)data)[0] = new_size;
    ((int*)data)[1] = LPX_ALLOC_SIGN1;
    actual_data = ((int*)data) + 2;
    ((int*)actual_data)[(new_size >> 2)] = LPX_ALLOC_SIGN2;
    LpxMemGlobalSafeAllocCount += new_size;
    return actual_data;
}

int LpxMemCheckOverflow(void * adr)
{
    int sign, size;
    if (adr == NULL)
        return -3;
    sign = *((int*)(adr) - 1);
    if (sign != LPX_ALLOC_SIGN1)
        return -1;
    size = *((int*)(adr) - 2);
    sign = ((int*)adr)[(size >> 2)];
    if (sign != LPX_ALLOC_SIGN2)
        return -2;
    return 0;
}

int LpxMemPoolCheckOverflow(void * adr)
{
    int sign, size, result;
    if ( (result = LpxMemCheckOverflow(adr)) != 0)
        return result;
    sign = *(int*)( (char*)adr + LpxMemGlobalPoolBlockSize);
    if (sign != LPX_POOL_SIGN)
        return -4;
    return 0;
}

void LpxMemSafeFree(void * adr)
{
    int size;
    assert(LpxMemCheckOverflow(adr) == 0);
    size = *((int*)(adr) - 2);
    LpxMemGlobalSafeFreeCount -= size;
    free((int*)adr - 2);
}

int LpxMemPoolInit(int block_size)
{
    if (LpxMemGlobalPoolInit || block_size < 0)
        return -1;
    LpxMemGlobalPoolInit = 1;
    LpxMemGlobalPoolBlockSize = block_size;
    LpxMemGlobalPoolAllocCount = 0;
    LpxMemGlobalPoolEmptyCount = 0;
    LpxListInit(&LpxMemGlobalPoolFreeList);
    LpxListInit(&LpxMemGlobalPoolAllocList);
    return 0;
}

void * LpxMemPoolAlloc()
{
    LpxList * list;
    void * pool_data;
    if (!LpxMemGlobalPoolInit)
        return NULL;
    list = LpxMemGlobalPoolFreeList.next;
    if (list == NULL)
    {
        //alloc new element
        pool_data = LpxMemSafeAlloc(LpxMemGlobalPoolBlockSize + sizeof(LpxList) + sizeof(int));
        if (pool_data == NULL)
            return NULL;
        *(int*)((char*)pool_data + LpxMemGlobalPoolBlockSize) = LPX_POOL_SIGN;
        list = (LpxList*)((char*)pool_data + LpxMemGlobalPoolBlockSize + sizeof(int));
        LpxListInit(list);
        LpxListAddTail(&LpxMemGlobalPoolAllocList, list);
        ++LpxMemGlobalPoolAllocCount;
        return pool_data;
    }
    LpxListRemove(list);
    --LpxMemGlobalPoolEmptyCount;
    LpxListAddTail(&LpxMemGlobalPoolAllocList, list);
    ++LpxMemGlobalPoolAllocCount;
    pool_data = ((char*)list) - LpxMemGlobalPoolBlockSize;
    return pool_data;
}

void LpxMemPoolFree(void * adr)
{
    LpxList * list;
    assert(LpxMemPoolCheckOverflow(adr) == 0);
    list = (LpxList*)((char*)adr + LpxMemGlobalPoolBlockSize + sizeof(int));
    LpxListRemove(list);
    --LpxMemGlobalPoolAllocCount; //it's possible to 'free' already free block
                                  //this will lead to mess the counters
    LpxListAddTail(&LpxMemGlobalPoolFreeList, list);
    ++LpxMemGlobalPoolEmptyCount;
}

void LpxMemPoolTrueFree(void * adr)
{
    LpxList * list;
    list = (LpxList*)((char*)adr + LpxMemGlobalPoolBlockSize);
    LpxListRemove(list);
    LpxMemSafeFree(adr);
}

