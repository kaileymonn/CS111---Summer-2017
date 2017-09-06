//NAME: Kai Wong
//EMAIL: kaileymon@g.ucla.edu
//ID: 704451679

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sched.h>

//------------------GLOBALS---------------------//
long long num_threads = 1;
long long num_iterations = 1;
long long counter = 0;

struct timespec start_time;
struct timespec end_time;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
long long spin_lock = 0;

char sync_flag;
int yield_flag = 0;
//-------------ADDITIONAL FUNCTIONS-------------//

//Basic add(2) function
void add(long long *pointer, long long value);
 
//Add function for thread
void *t_add();


//-----------------MAIN ROUTINE-----------------//

int main(int argc, char *argv[]) {
  
  int opt = 0;
  static struct option long_opts[] = {
    {"threads", required_argument, NULL, 't'},
    {"iterations", required_argument, NULL, 'i'},
    {"sync", required_argument, NULL, 's'},
    {"yield", no_argument, NULL, 'y'},
  };

  while((opt = getopt_long(argc, argv, "t:i:", long_opts, NULL)) != -1)
    {
      switch(opt)
	{
	case 't':
	  num_threads = atoi(optarg);
	  break;
	case 'i':
	  num_iterations = atoi(optarg);
	  break;
	case 'y':
	  //TODO
	  yield_flag = 1;
	  break;
	case 's':
	  //TODO
	  if(strlen(optarg) == 1 && optarg[0] == 'm') { sync_flag = 'm'; }
	  else if(strlen(optarg) == 1 && optarg[0] == 's') { sync_flag = 's'; }
	  else if(strlen(optarg) == 1 && optarg[0] == 'c') { sync_flag = 'c'; }
	  else {
	    perror("Invalid arguments used for sync option. m:s:c"); 
	    exit(1);
	  }
	  break;
	default:
	  perror("Invalid usage, try again");
	  exit(1);
	}
    }

  pthread_t *tids = malloc(num_threads * sizeof(pthread_t));
  
  //Start the clock
  if((clock_gettime(CLOCK_MONOTONIC, &start_time)) == -1) {
    perror("Failed to start timer"); 
    exit(2); 
  }

  //Create threads and join them
  long long i;
  long long j;
  for(i = 0; i < num_threads; i++) {pthread_create(&tids[i], NULL, t_add, NULL);}
  for(j = 0; j < num_threads; j++) {pthread_join(tids[j], NULL);}

  //Stop the clock 
  if(clock_gettime(CLOCK_MONOTONIC, &end_time) == -1) { 
    perror("Failed to stop timer");
    exit(2); 
  }
  
  //Free malloc-ed memory from heap
  free(tids);

  //Calculate total run time elapsed
  long long total_time = 1000000000 * (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec);
  
  //Total number of operations
  int num_operations = num_threads * num_iterations * 2;
  
  //Calculate average time per operation
  long long tpo = total_time / num_operations;
  
  long long info[6] = {num_threads, num_iterations, num_operations, total_time, tpo, counter};
  
  printf("add");
  if(yield_flag)
    printf("-yield");
  if(sync_flag == 'm')
    printf("-m");
  if(sync_flag == 's')
    printf("-s");
  if(sync_flag == 'c')
    printf("-c");
  if(!sync_flag)
    printf("-none");

  int k;
  for(k = 0; k < 6; k++){
    printf(",%lld", info[k]);
  }
  printf("\n");

  //Report nonzero counter value
  if(counter != 0) {
    //fprintf(stderr, "Error: Counter value is non-zero, counter = %lld\n", counter);
    exit(1);
    }
  
  exit(0);
  

}


//-----------------ADDITIONAL FUNCTION IMPLEMENTATIONS---------------------//
void add(long long *pointer, long long value) {
  //Set locks
  if(sync_flag == 'm') {pthread_mutex_lock(&mutex);}
  if(sync_flag == 's') {while(__sync_lock_test_and_set(&spin_lock, 1));}

  //Compare values
  long long sum = value + *pointer;
  if(yield_flag) {sched_yield();}
  *pointer = sum;

  //Release locks
  if(sync_flag == 's') {__sync_lock_release(&spin_lock);}
  if(sync_flag == 'm') {pthread_mutex_unlock(&mutex);}
}

void *t_add() {
  //Initialize local variables
  long long old = 0;
  long long new = 0;
  long long it = num_iterations;
  
  //Addition of counter by 1
  long long i;
  for(i = 0; i < it; i++) {
    //Compare and swap
    if(sync_flag == 'c') {
      do {
	old = counter;
	new = old + 1;
	if(yield_flag) {sched_yield();}
      } while(__sync_val_compare_and_swap(&counter, old, new) != old);
    }
    else {
      add(&counter, 1);
    }
  }
  
  //Decrement counter by 1
  long long j;
  for(j = 0; j < it; j++) {
    //Compare and swap
    if(sync_flag == 'c') {
      do {
	old = counter;
	new = old - 1;
	if(yield_flag) {sched_yield();}
      } while(__sync_val_compare_and_swap(&counter, old, new) != old);
    }
    else {
      add(&counter, -1);
    }
  }
  return NULL;
}
