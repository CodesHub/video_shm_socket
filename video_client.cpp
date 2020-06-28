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
#include "yuv_to_jpeg.h"
#include "unistd.h"
#include <opencv2/opencv.hpp> //opencv的头文件
#include "shm.h"
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <frame_protocol.h>
#include <sem.h>
#include <sys/sem.h>

using namespace cv; //使用命名空间cv,使用过C++的都明白，我们写C++程序必须使用using namespace std。

#define HOST "127.0.0.1"		 // 根据你服务器的 IP 地址修改
#define PORT 6667				 // 根据你服务器进程绑定的端口号修改
#define BUFFER_SIZ (1024 * 1024) // 4k 的数据区域
#define FPS 60					 //帧率

int image_width = 1280;
int image_height = 720;
unsigned char socket_buffer[BUFFER_SIZ];
unsigned char decode_buffer[BUFFER_SIZ];
unsigned int revLen = 0;
int sockfd = 0;

void stop(int);

int socket_client_init(void)
{
	int sockfd = -1, ret;
	struct sockaddr_in server;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket_server_init:");
		printf("create an endpoint for communication fail!\n");
		exit(1);
	}

	//设置接受缓冲区大小
	int recvbuf = 1024 * 1024;
	int len = sizeof(recvbuf);
	setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recvbuf, sizeof(recvbuf));
	getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recvbuf, (socklen_t *)&len);
	printf("the receive buffer size after setting is %d\n", recvbuf);

	// 建立 TCP 连接
	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);			  //将整型变量从主机字节顺序转变成网络字节顺序(大端)
	server.sin_addr.s_addr = inet_addr(HOST); //ip string 转换为32bit
	while (1)
	{
		if (connect(sockfd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
		{
			printf("等待服务器启动...\n");
		}
		else
		{
			break;
		}
		sleep(1);
	}

	printf("connect server success...\n");
	return sockfd;
}

int main(int argc, char **argv)
{
	int actRead = 0, readRes = 0;
	CAN_msg *pmsgTemp, *pmsg;
	clock_t start, end;
	uint32_t last_id=0;
	
	start = clock(); /*记录起始时间*/
	//捕获ctrl+c
	signal(SIGINT, stop);

	/*初始化socket*/
	sockfd = socket_client_init();
	printf("sockfd:%d\n", sockfd);

	while (1)
	{
		/*读取socket*/
		readRes = read(sockfd, socket_buffer, sizeof(socket_buffer));
		actRead += readRes;
		if (readRes <= 0)
		{
			perror("socket read");
			printf("server close...\n");
			break;
		}
		else
		{
			//printf("socket clien:收到%d Byte\n", readRes);
		}

		start = clock(); /*记录起始时间*/
		for (int i = 0; i < readRes; i++)
		{
			/*使用自定义协议解析，解决数据粘包*/
			pmsg = uart_rec_canmsg((uint8_t *)decode_buffer, (uint8_t)socket_buffer[i]);
			if (pmsg != NULL)
			{
				/*解析出原始数据*/
				printf("socket clien:数据帧id:%5d, 数据帧长度:%d B, 数据段长度:%d B\n", pmsg->id, pmsg->total_len, pmsg->data_len);

				if (pmsg->id != last_id)
				{
					last_id = pmsg->id;
					
					/*控制FPS*/
					end = clock(); /*记录结束时间*/
					double seconds = (double)(end - start) / CLOCKS_PER_SEC;
					fprintf(stderr, "uart_rec_canmsg Use time is: %.8f\n", seconds);
					seconds = (1000.0 / (double)FPS) - seconds * 1000.0;
					seconds = seconds > 1 ? seconds : 1;
					printf("need wait mseconds:%f\n", seconds);

					/*显示图片*/
					CvMat mCvmat = cvMat(image_width, image_height, CV_8UC1, pmsg->data);
					IplImage *IpImg = cvDecodeImage(&mCvmat, 1);
					//opencv3 .0 IplImage到Mat类型的转换的方法
					Mat image = cvarrToMat(IpImg);
					if (!image.data)
						return false;
					imshow("image", image);
					cvReleaseImage(&IpImg);
					waitKey(seconds);
				}
				actRead = actRead - pmsg->total_len;
				pmsg = NULL;
			}
		}
	}
	close(sockfd);
	return 0;
}

void stop(int)
{
	printf("\n关闭socket...\n");
	close(sockfd);
	_exit(0);
}
