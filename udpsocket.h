#ifndef UDPSOCKET_H
#define UDPSOCKET_H

#include <pthread.h>

#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<netdb.h>
#include<stdarg.h>
#include<string.h>
#include<errno.h>

#define SERVER_PORT 50101
#define BUFFER_SIZE 4096*500
#define FILE_NAME_MAX_SIZE 512

class udpsocket
{
public:
    udpsocket();
    ~udpsocket();
    void threadinit(int index);
    static void* udp_ts_recv1(void *param);
    static void* udp_ts_recv2(void *param);
    static void* udp_ts_recv3(void *param);
};

#endif // UDPSOCKET_H
