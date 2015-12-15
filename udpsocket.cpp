#include "udpsocket.h"

udpsocket::udpsocket()
{

}

udpsocket::~udpsocket()
{

}

void udpsocket::threadinit(int index)
{
    pthread_t udp_recv_thread;
    if(index==0){
        if( 0 != pthread_create( &udp_recv_thread, NULL, udp_ts_recv1, this ) )
            printf("%s:%d  Error: Create video encode thread failed !!!\n", __FILE__, __LINE__ );
    }
    else if(index==1){
        if( 0 != pthread_create( &udp_recv_thread, NULL, udp_ts_recv2, this ) )
            printf("%s:%d  Error: Create video encode thread failed !!!\n", __FILE__, __LINE__ );
    }
    else if(index==2){
        if( 0 != pthread_create( &udp_recv_thread, NULL, udp_ts_recv3, this ) )
            printf("%s:%d  Error: Create video encode thread failed !!!\n", __FILE__, __LINE__ );
    }

    pthread_detach(udp_recv_thread);
}

void *udpsocket::udp_ts_recv1(void * pArg)
{
    /* 创建UDP套接口 */
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //server_addr.sin_addr.s_addr = inet_addr("1.8.84.12");
    server_addr.sin_port = htons(50101);

    /* 创建socket */
    int server_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(server_socket_fd == -1)
    {
     perror("Create Socket Failed:");
     exit(1);
    }

    memset(server_addr.sin_zero,0,8);
     int addr_len = sizeof(struct sockaddr);
     int re_flag=1;
     int re_len=sizeof(int);
     setsockopt(server_socket_fd,SOL_SOCKET,SO_REUSEADDR,&re_flag,re_len);

    /* 绑定套接口 */
    if(-1 == (bind(server_socket_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))))
    {
     perror("Server Bind Failed:");
     exit(1);
    }
    printf("before server\n");
    /* 数据传输 */
    while(1)
    {
     /* 定义一个地址，用于捕获客户端地址 */
     struct sockaddr_in client_addr;
     socklen_t client_addr_length = sizeof(client_addr);
     /* 接收数据 */
     char buffer[BUFFER_SIZE];
     bzero(buffer, BUFFER_SIZE);
     /*if(recvfrom(server_socket_fd, buffer, BUFFER_SIZE,0,(struct sockaddr*)&client_addr, &client_addr_length) == -1)
     {
         printf("Receive Data Failed\n");
      //perror("Receive Data Failed:");
      //exit(1);
     }*/

    //int iSocketLen = sizeof(struct sockaddr_in);
     /* set recvfrom from server timeout */
     struct timeval tv;
     fd_set readfds;
     tv.tv_sec = 3;
     tv.tv_usec = 10;
     int iRet;
     FD_ZERO(&readfds);
     FD_SET(server_socket_fd, &readfds);
     select(server_socket_fd+1,&readfds,NULL, NULL, &tv);
     if (FD_ISSET(server_socket_fd,&readfds))
     {
         if (recvfrom(server_socket_fd, buffer, BUFFER_SIZE,0,(struct sockaddr*)&client_addr, &client_addr_length) == -1)
         {
             printf("received data error!\n");
             iRet=0;
         }
         else
             printf("socket 1 work\n");
     }
     else
     {
         printf("error is %d\n",errno);
         printf("timeout!there is no data arrived!\n");
         iRet=0;
     }

     /* 从buffer中拷贝出file_name */
     char file_name[FILE_NAME_MAX_SIZE+1];
     bzero(file_name,FILE_NAME_MAX_SIZE+1);
     strncpy(file_name, buffer, strlen(buffer)>FILE_NAME_MAX_SIZE?FILE_NAME_MAX_SIZE:strlen(buffer));
     //printf("%s\n", file_name);
    }
}

void *udpsocket::udp_ts_recv2(void * pArg)
{
    /* 创建UDP套接口 */
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //server_addr.sin_addr.s_addr = inet_addr("1.8.84.12");
    server_addr.sin_port = htons(50102);

    /* 创建socket */
    int server_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(server_socket_fd == -1)
    {
     perror("Create Socket Failed:");
     exit(1);
    }

    memset(server_addr.sin_zero,0,8);
     int addr_len = sizeof(struct sockaddr);
     int re_flag=1;
     int re_len=sizeof(int);
     setsockopt(server_socket_fd,SOL_SOCKET,SO_REUSEADDR,&re_flag,re_len);

    /* 绑定套接口 */
    if(-1 == (bind(server_socket_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))))
    {
     perror("Server Bind Failed:");
     exit(1);
    }
    /* 数据传输 */
    while(1)
    {
     /* 定义一个地址，用于捕获客户端地址 */
     struct sockaddr_in client_addr;
     socklen_t client_addr_length = sizeof(client_addr);
     /* 接收数据 */
     char buffer[BUFFER_SIZE];
     bzero(buffer, BUFFER_SIZE);
     /*if(recvfrom(server_socket_fd, buffer, BUFFER_SIZE,0,(struct sockaddr*)&client_addr, &client_addr_length) == -1)
     {
         printf("Receive Data Failed\n");
      //perror("Receive Data Failed:");
      //exit(1);
     }*/

    //int iSocketLen = sizeof(struct sockaddr_in);
     /* set recvfrom from server timeout */
     struct timeval tv;
     fd_set readfds;
     tv.tv_sec = 3;
     tv.tv_usec = 10;
     int iRet;
     FD_ZERO(&readfds);
     FD_SET(server_socket_fd, &readfds);
     select(server_socket_fd+1,&readfds,NULL, NULL, &tv);
     if (FD_ISSET(server_socket_fd,&readfds))
     {
         if (recvfrom(server_socket_fd, buffer, BUFFER_SIZE,0,(struct sockaddr*)&client_addr, &client_addr_length) == -1)
         {
             printf("received data error!\n");
             iRet=0;
         }
         else
             printf("socket 2 work\n");
     }
     else
     {
         printf("error is %d\n",errno);
         printf("timeout!there is no data arrived!\n");
         iRet=0;
     }

     /* 从buffer中拷贝出file_name */
     char file_name[FILE_NAME_MAX_SIZE+1];
     bzero(file_name,FILE_NAME_MAX_SIZE+1);
     strncpy(file_name, buffer, strlen(buffer)>FILE_NAME_MAX_SIZE?FILE_NAME_MAX_SIZE:strlen(buffer));
     //printf("%s\n", file_name);
    }
}

void *udpsocket::udp_ts_recv3(void * pArg)
{
    /* 创建UDP套接口 */
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //server_addr.sin_addr.s_addr = inet_addr("1.8.84.12");
    server_addr.sin_port = htons(50103);

    /* 创建socket */
    int server_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(server_socket_fd == -1)
    {
     perror("Create Socket Failed:");
     exit(1);
    }

    memset(server_addr.sin_zero,0,8);
     int addr_len = sizeof(struct sockaddr);
     int re_flag=1;
     int re_len=sizeof(int);
     setsockopt(server_socket_fd,SOL_SOCKET,SO_REUSEADDR,&re_flag,re_len);

    /* 绑定套接口 */
    if(-1 == (bind(server_socket_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))))
    {
     perror("Server Bind Failed:");
     exit(1);
    }
    /* 数据传输 */
    while(1)
    {
     /* 定义一个地址，用于捕获客户端地址 */
     struct sockaddr_in client_addr;
     socklen_t client_addr_length = sizeof(client_addr);
     /* 接收数据 */
     char buffer[BUFFER_SIZE];
     bzero(buffer, BUFFER_SIZE);
     /*if(recvfrom(server_socket_fd, buffer, BUFFER_SIZE,0,(struct sockaddr*)&client_addr, &client_addr_length) == -1)
     {
         printf("Receive Data Failed\n");
      //perror("Receive Data Failed:");
      //exit(1);
     }*/

    //int iSocketLen = sizeof(struct sockaddr_in);
     /* set recvfrom from server timeout */
     struct timeval tv;
     fd_set readfds;
     tv.tv_sec = 3;
     tv.tv_usec = 10;
     int iRet;
     FD_ZERO(&readfds);
     FD_SET(server_socket_fd, &readfds);
     select(server_socket_fd+1,&readfds,NULL, NULL, &tv);
     if (FD_ISSET(server_socket_fd,&readfds))
     {
         if (recvfrom(server_socket_fd, buffer, BUFFER_SIZE,0,(struct sockaddr*)&client_addr, &client_addr_length) == -1)
         {
             printf("received data error!\n");
             iRet=0;
         }
         else
             printf("socket 3 work\n");
     }
     else
     {
         printf("error is %d\n",errno);
         printf("timeout!there is no data arrived!\n");
         iRet=0;
     }

     /* 从buffer中拷贝出file_name */
     char file_name[FILE_NAME_MAX_SIZE+1];
     bzero(file_name,FILE_NAME_MAX_SIZE+1);
     strncpy(file_name, buffer, strlen(buffer)>FILE_NAME_MAX_SIZE?FILE_NAME_MAX_SIZE:strlen(buffer));
     //printf("%s\n", file_name);
    }
}

