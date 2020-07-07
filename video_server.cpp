#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <poll.h>
#include <stdlib.h>
#include "unistd.h"
#include "shm.h"
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <frame_protocol.h>
#include <sem.h>
#include <sys/sem.h>

#define HOST "127.0.0.1"		 // 根据你服务器的 IP 地址修改
#define PORT 6667				 // 根据你服务器进程绑定的端口号修改
#define BUFFER_SIZ (1024 * 1024) // 4k 的数据区域
#define FPS 30					 //帧率

int image_width = 1280;
int image_height = 720;
unsigned char buffer[BUFFER_SIZ];
unsigned char socket_buffer[BUFFER_SIZ];
unsigned int revLen = 0;
int sockfd = -1, connfd = -1;
pthread_mutex_t mutex;
unsigned int imageid = 0;

void stop(int);

int socket_server_init(void)
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

	//设置地址可重用
	/*未设置此项前，若服务端开启后，又关闭，此时sock处于TIME_WAIT状态，与之绑定的socket地址不可重用，而导致再次开启服务端失败。
		经过setsockopt设置之后， 即使处于TIME_WAIT些状态也可以立即被重用。*/
	int reuse = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	reuse = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));

	//设置发送缓冲区大小，系统都会自动将其加倍
	int sendbuf = 1024 * 1024;
	int len = sizeof(sendbuf);
	setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sendbuf, sizeof(sendbuf));
	getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sendbuf, (socklen_t *)&len);
	printf("the tcp send buffer size after setting is %d\n", sendbuf);

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

void *socket_write(void *arg)
{
	int connfd = *(char *)arg;
	int actwrite = 0, res = 0;
	unsigned int frameLen = 0;
	static unsigned int mageid_last = 0;
	while (1)
	{
		if (imageid == 0)
		{
			printf("等待图片采集...\n");
			sleep(1);
			continue;
		}

		/*FPS控制*/
		usleep(1000000 / FPS); //10ms 100HZ

		try
		{
			if (mageid_last == imageid)
				continue;
			mageid_last = imageid;

			/* 互斥锁上锁 */
			res = pthread_mutex_lock(&mutex);
			if (res)
			{
				printf("Thread shm_read lock failed\n");
				pthread_mutex_unlock(&mutex);
				pthread_exit(NULL);
			}

			/*socket发送图片帧，解决粘包使用自定义协议*/
			/*数据打包成自定义数据帧协议*/
			frameLen = pack_send_canbuf(imageid, (uint8_t *)socket_buffer, buffer, (uint32_t)revLen);
			actwrite = 0;
			while (actwrite < frameLen)
			{
				actwrite += write(connfd, socket_buffer + actwrite, frameLen - actwrite); //不定长发送
				if (actwrite < 0)
				{
					perror("socket write");
					throw - 1;
					break;
				}
				else
				{
					//printf("socket server:ID:%d, 数据帧长度%d B,  数据段长度%d B, 实际发送 %d B\n", imageid, frameLen, revLen, actwrite);
				}
			}

			/* 互斥锁解锁 */
			pthread_mutex_unlock(&mutex);
		}
		catch (int erro)
		{
			if (erro == -1)
			{
				printf("客户端断开,线程退出...\n");
				close(connfd);
				pthread_mutex_unlock(&mutex);
				pthread_exit(0);
			}
			else
			{
				printf("socket 异常\n");
			}
		}
	}

	close(connfd);
	printf("客户端断开,线程退出...\n");
	pthread_exit(0);
}

void *shm_read(void *arg)
{
	void *shm = NULL;
	int res = 0;

	//申请共享内存
	shm = shmInit(BUFFER_SIZ);

	while (1)
	{
		/* 互斥锁上锁 */
		res = pthread_mutex_lock(&mutex);
		if (res)
		{
			printf("Thread shm_read lock failed\n");
			pthread_mutex_unlock(&mutex);
			pthread_exit(NULL);
		}

		/*从共享内存读jpg 图片 */
		shmRead((char *)buffer, &revLen, (char *)shm);

		/* 互斥锁解锁 */
		pthread_mutex_unlock(&mutex);
		if (revLen != 0)
		{
			//printf("imageid:%d, size:%d\n", imageid, revLen);
			imageid++;
			fflush(stdout);
		}

		/*FPS*/
		usleep(1000000 / FPS); //10ms 100HZ
	}

	pthread_exit(0);
}

int main(int argc, char **argv)
{
	struct sockaddr_in client_addr;
	int len, pid;
	char client_ip[128];
	char arg_buf[1] = {0};
	socklen_t socketlen;
	pthread_t thread;
	void *thread_return;
	int res = 0;

	//捕获ctrl+c
	signal(SIGINT, stop);
	signal(SIGPIPE, SIG_IGN);

	/*初始化数据buffer互斥量*/
	pthread_mutex_init(&mutex, NULL);

	//开启shm获取图像线程
	res = pthread_create(&thread, NULL, shm_read, (void *)(arg_buf));
	if (res != 0)
	{
		printf("create thread fail\n");
		exit(res);
	}
	printf("create shm_read treads success\n");

	//初始化socket并监听
	sockfd = socket_server_init();

	while (1)
	{
		// 接受客户端socket连接
		socketlen = sizeof(client_addr);
		connfd = accept(sockfd, (struct sockaddr *)&client_addr, &socketlen);
		if (connfd < 0)
		{
			printf("server acccept failed...\n");
			raise(SIGINT);
		}
		printf("server acccept the client:%s\t%d\n",
			   inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, client_ip, sizeof(client_ip)),
			   ntohs(client_addr.sin_port));

		//多线程并发,开启socket发送线程 TODO
		arg_buf[0] = connfd;
		res = pthread_create(&thread, NULL, socket_write, (void *)(arg_buf));
		if (res != 0)
		{
			printf("create thread fail\n");
			exit(res);
		}
		printf("create socket_write treads success\n");
	}

	shmDetach();
	close(sockfd);
	pthread_mutex_destroy(&mutex);
	return 0;
}

void stop(int)
{
	//断开共享内存
	printf("\n关闭socket...\n");
	close(connfd);
	close(sockfd);
	printf("断开shm...\n");
	shmDetach();
	pthread_mutex_destroy(&mutex);
	_exit(0);
}