#include "lpx_args.h"
void LpxArgsParse(int argc, char ** argv, int * lport)
{
    int temp, len1, len2, i;
    extern int dbg_mode;
    extern int LpxMemGlobalFreeMode;
    char * username = NULL;
    char * password = NULL;
    char * auth_string = NULL;
    extern LpxConstString LpxGlobalPassData;
    * lport = 8080;
    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "-?") == 0))
    {
        printf("Loxy-Proxy V4 by Custom World\n"
               "available arguments:\n"
               "\t-l = listening port (8080 by default)\n"
               "\t-u = username for auth\n"
               "\t-p = password for auth\n"
               "\t-d = enable debug output\n"
               "\t-f = enable faster mem alloc\n");
    }
    for (i = 1; i < argc; i++)
    {
        //options w/o params
        if(strcmp(argv[i], "-f") == 0)
            LpxMemGlobalFreeMode = 1;
        if(strcmp(argv[i], "-d") == 0)
            dbg_mode = 1;
        if (i == argc - 1)
            break;
        //options with params
        if(strcmp(argv[i], "-l") == 0)
        {
            temp = atoi(argv[++i]);
            if (temp > 0 && temp < 65536)
            {
                * lport = temp;
                dbgprint(("ARGPARSE: listen port set to %d\n", *lport));
            }
        }
        if(strcmp(argv[i], "-u") == 0)
        {
            len1 = strlen(argv[++i]);
            if (len1 > 0)
            {
                username = (char*)LpxMemSafeAlloc(len1+1);
                if (username != NULL)
                {
                    strcpy(username, argv[i]);
                    dbgprint(("ARGPARSE: username set to %s\n", username));
                }
            }
        }
        if(strcmp(argv[i], "-p") == 0)
        {
            len2 = strlen(argv[++i]);
            if (len2 > 0)
            {
                password = (char*)LpxMemSafeAlloc(len2+1);
                if (password != NULL)
                {
                    strcpy(password, argv[i]);
                    dbgprint(("ARGPARSE: password set to %s\n", password));
                }
            }
        }
    }

    if (password != NULL && username != NULL)
    {
        temp = len1 + len2 + 1;
        auth_string = (char *)LpxMemSafeAlloc(temp);
        strcpy(auth_string, username);
        auth_string[len1] = ':';
        auth_string[len1 + 1] = 0;
        strcat(auth_string, password);
        dbgprint(("ARGPARSE: string to encode %s\n", auth_string));
        len1 = Base64encode_len(temp);
        LpxGlobalPassData.len = len1 - 1;
        LpxGlobalPassData.buf = (char*)LpxMemSafeAlloc(len1);
        Base64encode(LpxGlobalPassData.buf, auth_string, temp);
        dbgprint(("ARGPARSE: encoded string %s\n", LpxGlobalPassData.buf));
    }
    LpxMemSafeFree(password);
    LpxMemSafeFree(username);
    LpxMemSafeFree(auth_string);
}
