//main proxy source file. everything else comes from here.
#define _GNU_SOURCE
#include "lpx_mt.h"
#include "lpx_args.h"
#include "lpx_dbg.h"
//saved password auth info
LpxConstString LpxGlobalPassData = {NULL, 0};


int main(int argc, char ** argv)
{
    int listen_port;
    dbgprint(("debug mode enabled!\n"));
    //parse arguments
    LpxArgsParse(argc, argv, &listen_port);
    //init epoll
    
    //init listen socket
    
    //init signalfd for DNS resolving
    
    //enter main cycle
    
}
