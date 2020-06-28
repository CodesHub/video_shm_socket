#include <sys/types.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "sem.h"

char *shm = NULL; //分配的共享内存的原始首地址
int shmid;        //共享内存标识符
int semid;        //信号量标识符

void shmWrite(char *shm, char *buf, unsigned int len)
{
    unsigned int *dataSize = (unsigned int *)shm;
    *dataSize = len; //前四个字节为长度
    memcpy(shm + sizeof(unsigned int), buf, len);
    sem_v(semid); /* 释放信号量 */
}

void shmRead(char *buf, unsigned int *len, char *shm)
{
    if (sem_p(semid) == 0)
    {
        unsigned int *bufLen = (unsigned int *)shm;
        *len = *bufLen;
        memcpy(buf, shm + sizeof(unsigned int), (size_t)*len); //前四个字节为数据长度
    }
}

void*  shmInit(size_t size)
{
    //创建共享内存
    shmid = shmget((key_t)1234, size, 0666 | IPC_CREAT);
    if (shmid == -1)
    {
        fprintf(stderr, "shmget failed\n");
        exit(EXIT_FAILURE);
    }

    //将共享内存连接到当前进程的地址空间
    shm = shmat(shmid, 0, 0);
    if (shm == (void *)-1)
    {
        fprintf(stderr, "shmat failed\n");
        exit(EXIT_FAILURE);
    }
    printf("\nMemory attached at %p\n", shm);

    /** 打开信号量，不存在则创建 */
    semid = semget((key_t)6666, 1, 0666 | IPC_CREAT); /* 创建一个信号量 */
    if (semid == -1)
    {
        printf("sem open fail\n");
        exit(EXIT_FAILURE);
    }
    sem_init(semid, 0);

    return shm;
}

void shmDetach()
{
    //把共享内存从当前进程中分离
    if (shmdt(shm) == -1)
    {
        fprintf(stderr, "shmdt failed\n");
        exit(EXIT_FAILURE);
    }
}

void shmDel()
{
    sem_del(semid); /** 删除信号量 */
    //把共享内存从当前进程中分离
    if (shmdt(shm) == -1)
    {
        fprintf(stderr, "shmdt failed\n");
        exit(EXIT_FAILURE);
    }
    //删除共享内存
    if (shmctl(shmid, IPC_RMID, 0) == -1)
    {
        fprintf(stderr, "shmctl(IPC_RMID) failed\n");
        exit(EXIT_FAILURE);
    }
}
