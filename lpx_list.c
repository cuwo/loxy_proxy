#include "lpx_mem.h"

void LpxListInit(LpxList * list)
{
    list->next = list->prev = NULL;
}

void LpxListRemove(LpxList * list)
{
    if (list -> prev != NULL)
    {
        list -> prev -> next = list -> next;
    }
    if (list -> next != NULL)
    {
        list -> next -> prev = list -> prev;
    }
    list -> next = list -> prev = NULL;
}

void LpxListAddTail(LpxList * where, LpxList * what)
{
    if (what -> prev != NULL || what -> next != NULL)
        return;
    if (where -> next != NULL)
    {
        where -> next -> prev = what;
    }
    what -> prev = where;
    what -> next = where -> next;
    where -> next = what;
}

void LpxListAddHead(LpxList * where, LpxList * what)
{
    if (where -> prev != NULL)
    {
        where -> prev -> next = what;
    }
    what -> next = where;
    what -> prev = where -> prev;
    where -> prev = what;
}