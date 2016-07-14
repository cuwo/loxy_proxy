#include "lpx_args.h"
void LpxArgsParse(int argc, char ** argv, int * lport)
{
    int temp, len1, len2, i;
    char * username = NULL;
    char * password = NULL;
    char * auth_string = NULL;
    extern LpxConstString LpxGlobalPassData;
    * lport = 8080;
    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "-?") == 0))
    {
        printf("Loxy-Proxy V4 by Custom World\navailable arguments:\n\t-l = listening port (8080 by default)\n\t-u = username for auth\n\t-p = password for auth\n");
    }
    for (i = 1; i < argc-1; i++)
    {
        if(strcmp(argv[i], "-l") == 0)
        {
            temp = atoi(argv[i+1]);
            if (temp > 0 && temp < 65536)
            {
                * lport = temp;
                dbgprint(("ARGPARSE: listen port set to %d\n", *lport));
            }
        }
        if(strcmp(argv[i], "-u") == 0)
        {
            len1 = strlen(argv[i+1]);
            if (len1 > 0)
            {
                username = (char*)LpxMemSafeAlloc(len1+1);
                if (username != NULL)
                {
                    strcpy(username, argv[i+1]);
                    dbgprint(("ARGPARSE: username set to %s\n", username));
                }
            }
        }
        if(strcmp(argv[i], "-p") == 0)
        {
            len2 = strlen(argv[i+1]);
            if (len2 > 0)
            {
                password = (char*)LpxMemSafeAlloc(len2+1);
                if (password != NULL)
                {
                    strcpy(password, argv[i+1]);
                    dbgprint(("ARGPARSE: password set to %s\n", password));
                }
            }
        }
    }
    if (password != NULL && username != NULL)
    {
        dbgprint(("ARGPARSE: username got: %s\n", username));
        dbgprint(("ARGPARSE: password got: %s\n", password));
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
        dbgprint(("ARGPARSE: encoded string %s\n", upass));
    }
    LpxMemSafeFree(password);
    LpxMemSafeFree(username);
    LpxMemSafeFree(auth_string);
}
