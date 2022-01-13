#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>

#define REINDEERS 4
#define ELVES 4
#define MIN_ELVES 2

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t print_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t   cnd_started_giving_presents = PTHREAD_COND_INITIALIZER, 
                        cnd_finished_giving_presents = PTHREAD_COND_INITIALIZER, 
                        cnd_started_consulting  = PTHREAD_COND_INITIALIZER,
                        cnd_finished_consulting  = PTHREAD_COND_INITIALIZER,
                        cnd_wake_up_call = PTHREAD_COND_INITIALIZER,
                        cnd_santa_not_busy = PTHREAD_COND_INITIALIZER;

static volatile int reindeers_waiting = 0;
static volatile int elves_waiting = 0;
static volatile bool santa_delivering_presents = false;
static volatile bool santa_consulting = false;

void* santa_program(void* p){

   // santa is always waiting to synchronise with others
   // doesn't have "time off"
   while(true){
      pthread_mutex_lock( &mtx );

   
      while ( reindeers_waiting < REINDEERS && elves_waiting < MIN_ELVES ) {
         pthread_cond_wait( &cnd_wake_up_call, &mtx );
      }
   
      if(reindeers_waiting == REINDEERS){

         for(int i = 0; i < reindeers_waiting; i++){
            pthread_cond_signal( &cnd_started_giving_presents );
         }
         int reindeers_with_santa = reindeers_waiting;
         reindeers_waiting = 0;

         int delivery_time = 1 + (rand()%15);
         printf("\033[0;31mSanta:\033[0m \033[0;36mDelivering presents %is\033[0m\n", delivery_time);
         santa_delivering_presents = true;
         pthread_mutex_unlock( &mtx );

         sleep(delivery_time);

         pthread_mutex_lock( &mtx );
         santa_delivering_presents = false;
         for(int i = 0; i < reindeers_with_santa; i++){
            pthread_cond_signal( &cnd_finished_giving_presents );
         }
         
      }
      else if(elves_waiting >= MIN_ELVES){
         for(int i = 0; i < elves_waiting; i++){
            pthread_cond_signal( &cnd_started_consulting );
         }
         int elves_with_santa = elves_waiting;
         elves_waiting = 0;

         int consultation_time = 1 + (rand()%15);
         printf("\033[0;31mSanta:\033[0m \033[0;32mConsultations %is\033[0m\n", consultation_time);
         santa_consulting = true;
         pthread_mutex_unlock( &mtx );

         sleep(consultation_time);

         pthread_mutex_lock( &mtx );
         santa_consulting = false;
         for(int i = 0; i < elves_with_santa; i++){
            pthread_cond_signal(&cnd_finished_consulting);
         }   

      }

      for(int i = 0; i < ELVES + REINDEERS; i++){
         pthread_cond_signal(&cnd_santa_not_busy);
      }

      pthread_mutex_unlock( &mtx );
   }
   

}

void tabs(int id){
   pthread_mutex_lock( &print_mtx );
   for(int i = 1; i <= 4*id; i++){
      printf("\t");
   }
   printf("%i:", id);
   pthread_mutex_unlock( &print_mtx );
}

void* elf_program(void* p){
   int id = *(int*)p;

   while(true){
      int making_toys_time = 1 + (rand()%15);
      tabs(id);printf("\033[0;32mmaking toys %is\033[0m\n", making_toys_time);
      sleep(making_toys_time);
      tabs(id);printf("\033[0;32mfinsihed making toys\033[0m\n");




      pthread_mutex_lock( &mtx );
      while(santa_consulting){
         pthread_cond_wait( &cnd_santa_not_busy, &mtx );
      }
      elves_waiting++;
      tabs(id);printf("\033[0;32mwaiting for consulting - %i\033[0m\n", elves_waiting);
      pthread_cond_signal( &cnd_wake_up_call );

      pthread_cond_wait( &cnd_started_consulting, &mtx );
      tabs(id);printf("\033[0;32mconsulting with santa\033[0m\n");
      pthread_cond_wait( &cnd_finished_consulting, &mtx );

      
      tabs(id);printf("\033[0;32mfinshed consult\033[0m\n");
      pthread_mutex_unlock( &mtx );
   }
   
}

void* reindeer_program(void* p){
   int id = *(int*)p;

   while(true){
      int waiting_time = 1 + (rand()%15);
      tabs(id);printf("\033[0;36midle %is\033[0m\n", waiting_time);
      sleep(waiting_time);
      tabs(id);printf("\033[0;36mfnished idling\033[0m\n");



      pthread_mutex_lock( &mtx );
      while(santa_delivering_presents){
         pthread_cond_wait( &cnd_santa_not_busy, &mtx );
      }

      reindeers_waiting++;
      tabs(id);printf("\033[0;36mwaiting for delivery - %i\033[0m\n", reindeers_waiting);
      pthread_cond_signal( &cnd_wake_up_call );

      pthread_cond_wait( &cnd_started_giving_presents, &mtx );
      tabs(id);printf("\033[0;36mdelivering with santa\033[0m\n");
      pthread_cond_wait( &cnd_finished_giving_presents, &mtx );

      
      tabs(id);printf("\033[0;36mfinshed delivery\033[0m\n");
      pthread_mutex_unlock( &mtx );
   }
   
}


int main(){
   srand(time(NULL));
   //setbuf(stdout, NULL);

   pthread_t santa_thread;
   pthread_t reindeer_threads[REINDEERS];
   pthread_t elf_threads[ELVES];

   unsigned int thread_ids[1 + REINDEERS + ELVES];

   int thread_count = 0;
   thread_ids[thread_count] = thread_count;
   pthread_create(&santa_thread, NULL, santa_program, &thread_ids[thread_count]);
   thread_count++;

   for(int i = 0; i < REINDEERS; i++){
      thread_ids[thread_count] = thread_count;
      pthread_create(&reindeer_threads[i], NULL, reindeer_program, &thread_ids[thread_count]);
      thread_count++;
   }

   for(int i = 0; i < ELVES; i++){
      thread_ids[thread_count] = thread_count;
      pthread_create(&elf_threads[i], NULL, elf_program, &thread_ids[thread_count]);
      thread_count++;
   }

   pthread_join(santa_thread, NULL);
   return 0;

}

