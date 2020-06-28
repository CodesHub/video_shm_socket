#ifndef _SEM_H_
#define _SEM_H_

#ifdef __cplusplus
extern "C" {
#endif
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};
extern int sem_init(int sem_id, int init_value);
extern int sem_del(int sem_id);
extern int sem_p(int sem_id);
extern int sem_v(int sem_id);

#ifdef __cplusplus
}
#endif
#endif