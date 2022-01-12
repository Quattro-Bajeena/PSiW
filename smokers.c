#include <stdio.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define MAX 1000

static struct sembuf buf;

void podnies(int semid, int semnum){
   buf.sem_num = semnum;
   buf.sem_op = 1;
   buf.sem_flg = 0;
   if (semop(semid, &buf, 1) == -1){
      perror("Podnoszenie semafora");
      exit(1);
   }
}

void opusc(int semid, int semnum){
   buf.sem_num = semnum;
   buf.sem_op = -1;
   buf.sem_flg = 0;
   if (semop(semid, &buf, 1) == -1){
      perror("Opuszczenie semafora");
      exit(1);
   }
}


int main_smokers(){


    struct sembuf p0 = {0, -1, 0};
    struct sembuf p1 = {1, -1, 0};
    struct sembuf v0 = {0, 1, 0};
    struct sembuf v1 = {1, 1, 0};

    int semid = semget(2137, 2, IPC_CREAT|0600);
    if(semid == -1){
        perror("Utworzenie tablicy semaforow");
        exit(1);
    }
    semctl(semid, 0, SETVAL, 1);
    semctl(semid, 1, SETVAL, 0);

    int *buf;
    int shmid = shmget(123123, MAX*sizeof(int), IPC_CREAT|0600);
    if(shmid == -1){
        perror("Couldnt create shared memory");
        exit(1);

    }

    buf = (int*)shmat(shmid, NULL, 0);
    if(buf == NULL){
        perror("Couldnt attached shared memory");
        exit(1);
    }

    for(int i = 0; i < MAX; i++){
        buf[i] = 0;
    }

    printf("Before fork\n");
    if(fork() == 0){
        srand(getpid());
        for(int i = 1; i < MAX; i++){
            //while(*buf != 0){
            //  usleep(1000);
            //}
            semop(semid, &p0, 1);
            *buf = i;

            int sleepTime = rand()%10 * 1000;
            usleep(sleepTime);
            semop(semid, &v1, 1);
        }
        exit(1);
    }
    srand(getpid());
    for(int j = 1; j < MAX; j++){
        //while(*buf == 0){
        //  usleep(1000);
        //}
        semop(semid, &p1, 1);
        printf("%i ", *buf);
        fflush(stdout);
        //*buf = 0;
        int sleepTime = rand()%10 * 1000;
        usleep(sleepTime);
        semop(semid, &v0, 1);

    }

    return 0;

}


