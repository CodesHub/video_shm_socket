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

#define SHM_KEY (1234)
#define SEM_KEY (6666)

static char *shm = NULL; //分配的共享内存的原始首地址
static int shmid;        //共享内存标识符
static int semid;        //信号量标识符

void shmWrite(char *shm, char *buf, unsigned int len)
{
    if (sem_p(semid) == 0)
    {
        unsigned int *dataSize = (unsigned int *)shm;
        *dataSize = len; //前四个字节为长度
        memcpy(shm + sizeof(unsigned int), buf, len);
        sem_v(semid); /* 释放信号量 */
    }
    //printf("semval value:%d\n", semctl(semid, 0, GETVAL));
}

void shmRead(char *buf, unsigned int *len, char *shm)
{
    if (sem_p(semid) == 0)
    {
        unsigned int *bufLen = (unsigned int *)shm;
        *len = *bufLen;
        memcpy(buf, shm + sizeof(unsigned int), (size_t)*len); //前四个字节为数据长度
        sem_v(semid);                                          /* 释放信号量 */
    }
}

void *shmInit(size_t size)
{
    //创建共享内存
    shmid = shmget((key_t)SHM_KEY, size, 0666 | IPC_CREAT);
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
    printf("shm Memory attached at %p\n", shm);

    /** 打开信号量，不存在则创建 */
    semid = semget((key_t)SEM_KEY, 1, 0666 | IPC_CREAT); /* 创建一个信号量 */
    if (semid == -1)
    {
        printf("sem open fail\n");
        exit(EXIT_FAILURE);
    }
    sem_init(semid, 1);

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
    printf("断开共享内存...\n");
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
    printf("删除信号量和共享内存...\n");
}
