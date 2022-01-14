#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>

#define BUF_SIZE 10

static volatile int buf[BUF_SIZE];
static volatile int take_index;
static volatile int put_index;
static volatile int in_buffer;

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cnd_empty = PTHREAD_COND_INITIALIZER, cnd_full  = PTHREAD_COND_INITIALIZER;


void put(int x){
   pthread_mutex_lock( &mtx );
   while ( in_buffer >= BUF_SIZE ) {
      pthread_cond_wait( &cnd_empty, &mtx );
   }
   buf[put_index] = x;
   in_buffer++;
	put_index = (put_index+1)%BUF_SIZE;
   printf("P - in:%i | put: %i | take: %i\n", in_buffer, put_index, take_index);

   pthread_cond_signal( &cnd_full );
   pthread_mutex_unlock( &mtx );
}

int take(){
   int x;
   pthread_mutex_lock( &mtx );
   while ( in_buffer == 0 ) {
      pthread_cond_wait( &cnd_full, &mtx );
   }
   x = buf[take_index];
	in_buffer--;
	take_index = (take_index+1)%BUF_SIZE;
   printf("T - in:%i | put: %i | take: %i\n",in_buffer, put_index, take_index);

   pthread_cond_signal( &cnd_empty );
   pthread_mutex_unlock( &mtx );
   return x;
}

void* send(void* p){
	while(true){
      // generating value
		int num = rand_r( (unsigned int*)p ) % 100;
		int sleep_time = 1 + (rand_r( (unsigned int*)p ) % 4);
		sleep(sleep_time);

      // putting
		put(num);

	}
}

void* receive(void* p){
   int x;

	while(true){
		x = take();
   	printf("%d\n", x); fflush(stdout);

      // doing someting with value
      int sleep_time = 1 + (rand_r((unsigned int*)p)%2);
      
		sleep(sleep_time);
	}

}
int main(){
   srand(time(NULL));
   pthread_t tsnd_1, tsnd_2, tsnd_3, trcv;
   unsigned int seed0, seed1, seed2, seed3;

   seed0 = rand();seed1 = rand();seed2 = rand();seed3 = rand();
    
	in_buffer = 0;
	take_index = 0;
	put_index = 0;

	pthread_create(&tsnd_1, NULL, send, &seed1);
	pthread_create(&tsnd_2, NULL, send, &seed2);
   pthread_create(&tsnd_3, NULL, send, &seed3);
	pthread_create(&trcv, NULL, receive, &seed0);
    
	pthread_join(trcv, NULL);
	pthread_join(tsnd_1, NULL);
	pthread_join(tsnd_2, NULL);
   return 0;
}

