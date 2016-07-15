//debug functions I will use for this project
#pragma once
#include "includes.h"
#include "lpx_sd.h"
#define LPX_MAX_LISTEN 16
#define LPX_TIMER_PERIOD 5
#define LPX_MAX_EVENTS 32

//start listening for incoming connections
int LpxListenCompleteInit(int listen_port, int epoll_fd);

//init signalfd for DNS name resolving thing
int LpxDnsCompleteInit(int epoll_fd);

//init timer for timeout checking thing
int LpxTimerCompleteInit(int epoll_fd);