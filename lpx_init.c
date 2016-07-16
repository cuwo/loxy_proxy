#include "lpx_init.h"
int LpxListenCompleteInit(int listen_port, int epoll_fd)
{
    int listen_sock, temp;
    struct sockaddr_in listen_addr;
    SD * listen_sd;
    //create a socket for listening
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_sock < 0)
        return -1;
    //set it reusable
    temp = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &temp, sizeof(temp));
    LpxNetMakeNonbl(listen_sock);
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(listen_port); //use the port specified
    listen_addr.sin_addr.s_addr = INADDR_ANY; //listen on all interfaces
    if (bind(listen_sock, (struct sockaddr*)&listen_addr, sizeof(listen_addr)) != 0) 
    {
        perror("bind failed");
        close(listen_sock);
        return -1;
    }
    if (listen(listen_sock, LPX_MAX_LISTEN) < 0) 
    {
        perror("listen failed");
        close(listen_sock);
        return -1;
    }
    listen_sd = LpxSdCreate();
    if (listen_sd == NULL)
    {
        close(listen_sock);
        return -1;
    }
    LpxSdInit(listen_sd, listen_sock, LPX_FLAG_OPEN | LPX_FLAG_MAINT | LPX_FLAG_LISTEN, 
                epoll_fd, EPOLLIN | EPOLLET);
    return 0;
}

int LpxDnsCompleteInit(int epoll_fd)
{
    int sigfd_sock;
    SD * sigfd_sd;
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, LPX_SIGNAL);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    sigfd_sock = signalfd(-1, &mask, SFD_NONBLOCK);
    if (sigfd_sock < 0)
        return -1;
    sigfd_sd = LpxSdCreate();
    if (sigfd_sd == NULL)
    {
        close(sigfd_sock);
        return -1;
    }
    LpxSdInit(sigfd_sd, sigfd_sock, LPX_FLAG_OPEN | LPX_FLAG_MAINT | LPX_FLAG_SIGFD, 
                epoll_fd, EPOLLIN | EPOLLET);
    return 0;
}

int LpxTimerCompleteInit(int epoll_fd)
{
    int timerfd;
    SD * timer_sd;
    struct itimerspec timer_val;
    timerfd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
    if (timerfd < 0)
        return -1;
    timer_val.it_value.tv_sec = LPX_TIMER_PERIOD;
    timer_val.it_value.tv_nsec = 0;
    timer_val.it_interval.tv_sec = LPX_TIMER_PERIOD;
    timer_val.it_interval.tv_nsec = 0;
    if ( timerfd_settime(timerfd, 0, &timer_val, NULL) < 0 ) 
    {
        close(timerfd);
        return -1;
    }
    timer_sd = LpxSdCreate();
    if (timer_sd == NULL)
    {
        close(timerfd);
        return -1;
    }
    LpxSdInit(timer_sd, timerfd, LPX_FLAG_OPEN | LPX_FLAG_MAINT | LPX_FLAG_TIMER, 
                epoll_fd, EPOLLIN | EPOLLET);
    return 0;
}
