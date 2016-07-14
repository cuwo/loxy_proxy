#include "lpx_dbg.h"

//snippet by epatel
void dbghex(void * data, int size)
{
#ifdef DEBUG
    unsigned char *buf = (unsigned char*)data;
    int i, j;
    for (i=0; i<size; i+=16) 
    {
        printf("%06x: ", i);
        for (j=0; j<16; j++) 
            if (i+j < buflen)
                printf("%02x ", buf[i+j]);
            else
                printf("   ");
        printf(" ");
        for (j=0; j<16; j++) 
            if (i+j < buflen)
                printf("%c", isprint(buf[i+j]) ? buf[i+j] : '.');
        printf("\n");
    }
#endif
}
