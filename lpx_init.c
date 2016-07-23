#include "lpx_init.h"

char * LpxErrorPrepare(const char * err, const char * text, const char * additional_header)
{
    int len;
    extern char * temp_buf;
    char * temp_2 = temp_buf + LPX_SD_SIZE/2;
    strcpy(temp_2, text);
    len = strlen(temp_2);
    sprintf(temp_buf, "HTTP/1.0 %s\r\nContent-Type: text/html\r\nContent-Length: %d\r\n"
    "Content-Language: en\r\nConnection: keep-alive%s\r\n\r\n%s", err, len, additional_header, temp_2);
    return temp_buf;
}

void LpxErrorInit()
{
    LpxConstStringInit(&LpxEstGlobal, "HTTP/1.1 200 Connection established\r\nLoxy-Proxy v4\r\n\r\n");
    LpxConstStringInit(&LpxErrGlobal400, LpxErrorPrepare("400 Bad Request", 
    "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n<html><head>\n<title>400 Bad Request</title>\n</head><body>\n<h1>Bad Request</h1>"
    "<p>Loxy-Proxy has failed to parse the HTTP request. Probably there is something wrong.</p>\n<hr>\n<address>Loxy-Proxy v4</address>\n</body></html>\n",
    ""));
    LpxConstStringInit(&LpxErrGlobal407, LpxErrorPrepare("407 Proxy Authentication Required", 
    "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n<html><head>\n<title>407 Proxy Authentication Required</title>\n</head><body>\n<h1>Authentication Required</h1>"
    "<p>Loxy-Proxy requires authentification to process your request. Please, enter your credentials.</p>\n<hr>\n<address>Loxy-Proxy v4</address>\n</body></html>\n",
    "\r\nProxy-Authenticate: Basic realm=\"Loxy-Proxy v4\"")); 
    LpxConstStringInit(&LpxErrGlobal503, LpxErrorPrepare("500 Internal Server Error", 
    "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n<html><head>\n<title>500 Internal Server Error</title>\n</head><body>\n<h1>Server Error</h1>"
    "<p>Something really wrong happened. Contact the proxy owner.</p>\n<hr>\n<address>Loxy-Proxy v4</address>\n</body></html>\n",
    ""));    
    LpxConstStringInit(&LpxErrGlobal503, LpxErrorPrepare("503 Service Unavailable", 
    "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n<html><head>\n<title>503 Service Unavailable</title>\n</head><body>\n<h1>Service Unavailable</h1>"
    "<p>Loxy-Proxy has failed to find the requested domain. Probably you made the mistake in the address part.</p>\n<hr>\n<address>Loxy-Proxy v4</address>\n</body></html>\n",
    ""));
    LpxConstStringInit(&LpxErrGlobal504, LpxErrorPrepare("504 Gateway Timeout", 
    "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n<html><head>\n<title>504 Gateway Timeout</title>\n</head><body>\n<h1>Gateway Timeout</h1>"
    "<p>Loxy-Proxy has failed to connect to the requested address. Probably it's down.</p>\n<hr>\n<address>Loxy-Proxy v4</address>\n</body></html>\n",
    ""));

}

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
