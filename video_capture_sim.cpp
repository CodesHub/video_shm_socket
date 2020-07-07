#include "unistd.h"
#include <stdio.h>
#include <sys/types.h>
#include "shm.h"
#include <pthread.h>
#include <signal.h>

#define BUFFER_SIZ (1024 * 1024) //数据区域
void *shm = NULL;
unsigned char buffer[BUFFER_SIZ];

void stop(int)
{
    //删除共享内存
    printf("\n删除shm...\n");
    shmDel();
    _exit(0);
}

int main(int argc, char **argv)
{
    int size = 0, i = 0;
    char fileName[128];
    //申请共享内存
    shm = shmInit(BUFFER_SIZ);

    //捕获ctrl+c
    signal(SIGINT, stop);

    while (1)
    {
        for (i = 0; i < 12; i++)
        {
            sprintf(fileName, "/home/jinhong/video_shm_socket/yuv_640_480/%d.yuv", i);
            FILE *fd = fopen(fileName, "r+b");
            if (NULL == fd) //判断指向我文件位置不为空
            {
                printf("打开图像文件失败!\n");
                return false;
            }
            size = fread(buffer, sizeof(char), BUFFER_SIZ, fd);
            printf("file read %d\r", size);
            fflush(stdout);
            fclose(fd);

            shmWrite((char *)shm, (char *)buffer, size);
            usleep(1000000 / 100);
        }
    }
    return 0;
}