#ifndef __SHM__
#define __SHM__

#ifdef __cplusplus
extern "C" {
#endif
void shmWrite(char *shm, char *buf, unsigned int len);
void shmRead(char *buf, unsigned int *len, char *shm);
void*  shmInit(size_t size);
void shmDetach();
void shmDel();
#ifdef __cplusplus
}
#endif

#endif
