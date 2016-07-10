#pragma once

//simple double-linked list

typedef struct LpxList
{
    struct LpxList * next;
    struct LpxList * prev;
} LpxList;

//init the list element
void LpxListInit(LpxList * list);

//cut all the links
void LpxListRemove(LpxList * list);

//add new element into list after the current
void LpxListAddTail(LpxList * where, LpxList * what);

//add new element into list before the current
void LpxListAddHead(LpxList * where, LpxList * what);