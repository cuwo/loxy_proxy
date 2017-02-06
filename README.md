# loxy_proxy
simple http proxy with good performance  

HOW_TO_USE:  

I use nohup to prevent closing after SSH disconnect  

-l <port>  (default 8080)  

-u <username>  
-p <password>  

example:  

nohup ./proxy -l 8080 -u user -p password  

the design ideas are complete  
the implementation is still in progress

current progress: 95%

TODO:

- find bugs
-  fix bugs

DONE:

- close link on timeout
- memory pool allocation
- epoll cycle for better performance
- password authentification
- http HEAD/GET/POST/CONNECT
- DNS name resolving via signalfd
