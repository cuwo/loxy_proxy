#pragma once
#define _GNU_SOURCE
#include <assert.h>
#include "errno.h"
#include "stdlib.h"
#include "stdio.h"
#include "stddef.h"
#include "string.h"
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/signalfd.h>
#include <sys/types.h>
#include <sys/timerfd.h>
#include <sys/time.h>
#include <inttypes.h>


#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))