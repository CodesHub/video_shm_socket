#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#define HOST "192.168.0.217"  // 根据你服务器的 IP 地址修改
#define PORT 6666             // 根据你服务器进程绑定的端口号修改
#define BUFFER_SIZ (4 * 1024) // 4k 的数据区域

static char buffer[BUFFER_SIZ]; //用于保存输入的文本

int socket_server_init()
{
    int sockfd;
    struct sockaddr_in server;
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        printf("socket creation failed...\n");
        exit(0);
    }
    printf("socket successfully created..\n");

    // assign IP, PORT
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);

    // binding newly created socket to given IP and verification
    if ((bind(sockfd, (struct sockaddr *)&server, sizeof(server))) != 0)
    {
        printf("socket bind failed...\n");
        exit(0);
    }
    printf("socket successfully binded..\n");

    // now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0)
    {
        printf("Listen failed...\n");
        exit(0);
    }
    printf("server start listening...\n");

    return sockfd;
}

void socket_server_accept(int sockfd)
{
    struct sockaddr_in client_addr;
    int connfd, len, pid;
    char client_ip[128];

    while (1)
    {
        // accept the data packet from client and verification
        len = sizeof(client_addr);
        connfd = accept(sockfd, (struct sockaddr *)&client_addr, &len);
        if (connfd < 0)
        {
            printf("server acccept failed...\n");
            exit(0);
        }
        printf("server acccept the client:%s\t%d\n",
               inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, client_ip, sizeof(client_ip)),
               ntohs(client_addr.sin_port));

        pid = fork();
        if (pid == 0)
        {
            //in child
            Close(sockfd);
            while (1)
            {
                len = Read(connfd, buf, sizeof(buf));
                if (len <= 0)
                    break;
                Write(STDOUT_FILENO, buf, len);
                for (i = 0; i < len; ++i)
                    buf[i] = toupper(buf[i]);
                Write(connfd, buf, len);
            }
            Close(connfd);
            return 0;
        }
        else if (pid > 0)
        {
            //in parent
            Close(connfd);
        }
        else
        {
            perror("fork");
            exit(1);
        }
    }
    close(sockfd);
}

int socket_client_init(void)
{
    int sockfd, ret;
    struct sockaddr_in server;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket_server_init:");
        printf("create an endpoint for communication fail!\n");
        exit(1);
    }
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);            //将整型变量从主机字节顺序转变成网络字节顺序(大端)
    server.sin_addr.s_addr = inet_addr(HOST); //ip string 转换为32bit
    // 建立 TCP 连接
    if (connect(sockfd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        printf("connect server fail...\n");
        close(sockfd);
        exit(1);
    }
    printf("connect server success...\n");
    return sockfd;
}

void socket_close(int sockfd)
{
    if (sockfd != -1)
        close(sockfd);
    else
        printf("socket_close:Invalid Parameter\n");
}

int main(void)
{
    int sockfd, ret;
    struct sockaddr_in server;

    // 创建套接字描述符
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("create an endpoint for communication fail!\n");
        exit(1);
    }
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr(HOST);
    // 建立 TCP 连接
    if (connect(sockfd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        printf("connect server fail...\n");
        close(sockfd);
        exit(1);
    }
    printf("connect server success...\n");
    while (1)
    {
        printf("please enter some text: ");
        fgets(buffer, BUFFER_SIZ, stdin);
        //输入了 end，退出循环（程序）
        if (strncmp(buffer, "end", 3) == 0)
            break;
        write(sockfd, buffer, sizeof(buffer));
    }
    close(sockfd);
    exit(0);
}