#include <stdio.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <limits.h>

#define PHILOSOPHERS_NO 5
#define MEAL_WEIGHT 100

static struct sembuf buf;

void signal(int semid, int semnum){
   buf.sem_num = semnum;
   buf.sem_op = 1;
   buf.sem_flg = 0; 
   if (semop(semid, &buf, 1) == -1){
      perror("Podnoszenie semafora");
      exit(1);
   }
}

void wait(int semid, int semnum){
   buf.sem_num = semnum;
   buf.sem_op = -1;
   buf.sem_flg = 0;
   if (semop(semid, &buf, 1) == -1){
      perror("Opuszczenie semafora");
      exit(1);
   }
}



struct Philosopher{
    int id;
    int amount_eaten;
    bool ready_to_eat;
};

void sort(struct Philosopher sorted_que[]){
    // maybe I should use sort that preserves order if the values are equal?
    bool swapped = false;
    int sorted_i = PHILOSOPHERS_NO;
    do{
        swapped = false;
        for(int current_i = 0; current_i < sorted_i - 1; current_i++){

            if(sorted_que[current_i].amount_eaten > sorted_que[current_i + 1].amount_eaten){

                struct Philosopher temp = sorted_que[current_i];
                sorted_que[current_i] = sorted_que[current_i + 1];
                sorted_que[current_i + 1] = temp;
                swapped = true;
            }   
        }
        sorted_i--;

    }while(swapped == true);

    // maybe sort once again but take all with id == -1 to the end
}

void enque(struct Philosopher sorted_que[], struct Philosopher newPhilosopher ){
    bool found_place = false;
    for(int i = 0; i < PHILOSOPHERS_NO; i++){
        if(sorted_que[i].id == -1){
            sorted_que[i] = newPhilosopher;
            found_place = true;
            break;
        }
    }
    if(found_place == false){
        perror("que is full!");
    }

    sort(sorted_que);

}

bool is_que_empty(struct Philosopher sorted_que[]){
    bool empty = true;
    for(int i = 0; i < PHILOSOPHERS_NO; i++){
        if(sorted_que[i].id != -1){
            empty = false;
            break;
        }
    }

    return empty;
}

struct Philosopher deque(struct Philosopher sorted_que[]){
    if(sorted_que[0].id == -1){
        perror("no philosophers in que!");
    }
    struct Philosopher first_p = sorted_que[0];

    // shidting que one place 
    for(int i = 0; i < PHILOSOPHERS_NO-1; i++){
        sorted_que[i] = sorted_que[i+1];
    }

    struct Philosopher null_philosopher = {-1,INT_MAX, false};
    sorted_que[PHILOSOPHERS_NO-1] = null_philosopher;

    sort(sorted_que);

    return first_p;
    
}


void phil_print(int id){
    id++;
    for(int i = 1; i <= 4*id; i++){
        printf("\t");
    }
}

void philosopher_prog(
    int id, 
    int waiter_permission_sem, 
    int philosophers_sem, int spoon_status_sem,
    struct Philosopher philosophers[], bool spoons[]){

    const int left_spoon = id;
    const int right_spoon = (id + 1) % PHILOSOPHERS_NO;
    int sleepTime;
    srand(getpid());
    while(true){

        // thinking
        sleepTime = (rand()%20);
        phil_print(id);printf("%i: started\033[0;32m THINKing\033[0m %is\n", id, sleepTime);
        sleep(sleepTime);
        phil_print(id);printf("%i: stopped\033[0;32m THINKing\033[0m\n", id);
        

        // ready to eat
        wait(philosophers_sem, id);
        philosophers[id].ready_to_eat = true;
        signal(philosophers_sem, id);

        // waiting for waiter permission 
        phil_print(id);printf("%i: waiting for\033[0;33m PERMISSION\033[0m\n", id);
        wait(waiter_permission_sem, id); 

        // eating
        sleepTime = (rand()%20);
        phil_print(id);printf("%i: started\033[0;31m EATing\033[0m %is\n", id, sleepTime);
        sleep(sleepTime);

        // finsihed eating
        wait(spoon_status_sem, 0);
        spoons[left_spoon] = true;
        spoons[right_spoon] = true;
        signal(spoon_status_sem, 0);

        wait(philosophers_sem, id);
        philosophers[id].amount_eaten += MEAL_WEIGHT;
        signal(philosophers_sem, id);
        
        phil_print(id);printf("%i: stopped\033[0;31m EATing\033[0m, eaten: %i\n", id, philosophers[id].amount_eaten);


    }
    
}


void print_que(int spoon_num, struct Philosopher* que){
    printf("\033[0;36m que %i: ", spoon_num);

    int i = 0;
    int phil_id = que[i].id;
    while(phil_id != -1){
        printf("%i(%i)", phil_id, que[i].amount_eaten);
        i++;
        phil_id = que[i].id;
    }
    printf("\033[0m\n");
}

void waiter_prog(
    int waiter_permission_sem, 
    int philosophers_sem, int spoon_status_sem,
    struct Philosopher philosophers[], bool spoons[])
    {

    // every spoon has its own que
    struct Philosopher* spoon_queues[PHILOSOPHERS_NO];

    for(int i = 0 ; i < PHILOSOPHERS_NO; i++){
        spoon_queues[i] = (struct Philosopher*)malloc(PHILOSOPHERS_NO * sizeof(struct Philosopher));

        for(int j = 0 ; j < PHILOSOPHERS_NO; j++){
            spoon_queues[i][j].id = -1; //-1 == empty place in que
            spoon_queues[i][j].amount_eaten = INT_MAX; //kinda hack I guess to philospher somehow eat so much as INT_MAX
            spoon_queues[i][j].ready_to_eat = false;
        }
        
    }

    while(true){

        // checking if someone wants to eat, enquing
        for(int id = 0; id < PHILOSOPHERS_NO; id++){

            wait(philosophers_sem, id);

            if(philosophers[id].ready_to_eat){
                const int left_spoon = id;
                const int right_spoon = (id + 1) % PHILOSOPHERS_NO;

                enque(spoon_queues[left_spoon], philosophers[id]); 
                enque(spoon_queues[right_spoon], philosophers[id]); 

                philosophers[id].ready_to_eat = false;
                printf("\033[0;36m %i enqued\033[0m\n", id);
                print_que(left_spoon, spoon_queues[left_spoon]);
                print_que(right_spoon, spoon_queues[right_spoon]);

            }

            signal(philosophers_sem, id);
        }

        // processing queues
        for(int id = 0; id < PHILOSOPHERS_NO; id++){
            const int left_spoon = id;
            const int right_spoon = (id + 1) % PHILOSOPHERS_NO;

            wait(spoon_status_sem, 0);
            // to be able to eat you must be first in que for both spoons and those spoons have to be free to use
            if(spoon_queues[left_spoon][0].id == id && spoon_queues[right_spoon][0].id == id && spoons[left_spoon] && spoons[right_spoon]){
                deque(spoon_queues[left_spoon]);
                deque(spoon_queues[right_spoon]);

                spoons[left_spoon] = false;
                spoons[right_spoon] = false;

                signal(waiter_permission_sem, id);
                printf("\033[0;36m waiter: %i allowed to eat\033[0m\n", id);
                // philosopher with id starts eating
            }

            signal(spoon_status_sem, 0);
        }

    }

}


int main(){
    int philosophers_shmid = shmget(1, PHILOSOPHERS_NO*sizeof(struct Philosopher), IPC_CREAT|0600);
    struct Philosopher* philosophers = (struct Philosopher*)shmat(philosophers_shmid, NULL, 0);

    // spoon false -> in use, true -> free to use
    int spoons_shmid = shmget(2, PHILOSOPHERS_NO*sizeof(bool), IPC_CREAT|0600);
    bool* spoons = (bool*)shmat(spoons_shmid, NULL, 0);


    int spoon_status_sem = semget(1, 1, IPC_CREAT|0600);
    semctl(spoon_status_sem, 0, SETVAL, 1);

    int waiter_permission_sem = semget(2, PHILOSOPHERS_NO, IPC_CREAT|0600);
    int philosophers_sem = semget(3, PHILOSOPHERS_NO, IPC_CREAT|0600);
    
    
    for(int i = 0 ; i < PHILOSOPHERS_NO; i++){
        struct Philosopher p = {
            i, 0, false
        };
        philosophers[i] = p;
        spoons[i] = true;
        
        semctl(waiter_permission_sem, i, SETVAL, 0);
        semctl(philosophers_sem, i, SETVAL, 1);
    }


    for(int i = 0; i < PHILOSOPHERS_NO; i++){

        pid_t pid = fork();
        if(pid == 0){
            philosopher_prog(i,waiter_permission_sem, philosophers_sem,spoon_status_sem, philosophers, spoons);
            exit(0);
        }
    }

    
    pid_t pid = fork();
    if(pid == 0){
        waiter_prog(waiter_permission_sem, philosophers_sem,spoon_status_sem, philosophers, spoons);
        exit(0);
    }

    return 0;
}




