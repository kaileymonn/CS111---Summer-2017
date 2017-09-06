//NAME: Kai Wong
//EMAIL: kaileymon@g.ucla.edu
//ID: 704451679

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <getopt.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include "SortedList.h"

//----------------GLOBALS-----------------//

//Option arguments
int num_threads = 1;
int num_iterations = 1;
int num_lists = 1;

//Option flags
int opt_yield = 0;
char opt_sync = 'd';

//Lock array initializations
pthread_mutex_t* mutex_locks;
int* spin_locks; 

//Time initializations
long long* thread_wait_times;

//List initializations
int* offset;
int list_len;
int num_elements; 
SortedList_t* sub_lists; //Array of sub-lists 
SortedListElement_t* elements; //All elements in the list, with keys 

//Strings
static char* CHAR_ARRAY = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
char test_name[15] = "list-";

//-----------ADDITIONAL FUNCTIONS---------//


//Thread routine
void* thread_function(void *arg);


//-------------MAIN ROUTINE--------------//

int main(int argc, char**argv) {
  srand(time(NULL));
  int ch;
  
  static struct option long_opts[] = {
    {"threads", required_argument, NULL, 't'},
    {"iterations", required_argument, NULL, 'i'},
    {"sync", required_argument, NULL, 's'},
    {"yield", required_argument, NULL, 'y'},
    {"lists", required_argument, NULL, 'l'},
  };


  while((ch = getopt_long(argc, argv, "t:i:l:s:y:", long_opts, NULL)) != -1) {
    switch(ch) 
      {
      case 'l':
	num_lists = atoi(optarg);
	break;
      
      case 'i':
	num_iterations = atoi(optarg);
	break;
      
      case 't':
	num_threads = atoi(optarg);
	break;
      
      case 's':
	opt_sync = optarg[0];
	if(opt_sync != 'm' && opt_sync != 's'){
	  perror("Invalid sync arguments");
	  exit(1);
	}
	break;

      case 'y':
	if (strlen(optarg) == 0 || strlen(optarg) > 3) {
	  perror("Invalid yield arguments");
	  exit(1);
	}
	strcat(test_name,optarg);

	for(int i = 0; i < strlen(optarg); i++) {
	  if (optarg[i] == 'i')
	    opt_yield |= INSERT_YIELD;
	  else if (optarg[i] == 'd')
	    opt_yield |= DELETE_YIELD;
	  else if (optarg[i] == 'l')
	    opt_yield |= LOOKUP_YIELD;
	  else {
	    perror("Invalid yield arguments");
	    exit(1);
	  }
	}
	break;
      
      default:
	perror("Invalid usage, try again");
	exit(1);
      }
  }

  //Initialize header nodes in sub-list array
  sub_lists = malloc(sizeof(SortedList_t) * num_lists);
  for(int i = 0; i < num_lists; i++) {
    sub_lists[i].key = NULL;
    sub_lists[i].next = &sub_lists[i];
    sub_lists[i].prev = &sub_lists[i];
  }

  //Initialize mutex lock and spin-lock arrays
  if(opt_sync == 'm') {
    mutex_locks = malloc(sizeof(pthread_mutex_t) * num_lists);
    for(int i = 0; i < num_lists; i++) { pthread_mutex_init(&mutex_locks[i], NULL); }
  }
  else if(opt_sync == 's') {
    spin_locks = malloc(sizeof(int) * num_lists);
    for(int i = 0; i < num_lists; i++) { spin_locks[i] = 0; }    
  }

  //Thread array initializations
  int* tid = malloc(sizeof(int) * num_threads);
  pthread_t* threads = malloc(sizeof(pthread_t) * num_threads);
  thread_wait_times = malloc(sizeof(long long) * num_threads);

  //Initialize elements and offset arrays, hash keys
  num_elements = num_iterations * num_threads;
  elements = malloc(sizeof(SortedListElement_t) * num_elements);
  offset = malloc(sizeof(int) * num_elements);
  

  //HASH
  //Find key_lengths for each new element's key
  for(int i = 0; i < num_elements; i++) {
    int key_length = (rand() % 5) + 10; 
    int position = 0;
    char* key = malloc((key_length + 1) * sizeof(char));
    
    //Assign key values
    for(int j = 0; j < key_length; j++){
      position = rand() % strlen(CHAR_ARRAY);
      key[j] = CHAR_ARRAY[position];
    }

    key[key_length] = '\0';
    elements[i].key = key;
  }


  //Create and run threads, record elapsed time
  struct timespec start_time;
  struct timespec end_time;
  clock_gettime(CLOCK_MONOTONIC, &start_time);
  for (int i = 0; i < num_threads; i++) {
    tid[i] = i;
    if(pthread_create(&threads[i], NULL, thread_function, &(tid[i])) < 0) {
      perror("Failed to pthread_create()"); 
      exit(1);
    }
  }

  for(int i = 0; i < num_threads; i++) {
    if(pthread_join(threads[i], NULL) < 0) {
      perror("Failed to pthread_join()"); 
      exit(1);
    }
  }
  clock_gettime(CLOCK_MONOTONIC, &end_time);

  
  //Check list length
  list_len = 0;
  for(int i = 0; i < num_lists; i++){
    list_len += SortedList_length(&(sub_lists[i]));
  }
  if(list_len < 0){
    fprintf(stderr, "Failed somewhere...Invalid list length, length = %d\n", list_len);
    exit(2);
  }

  //Elapsed time computations
  long long time_difference = ((end_time.tv_sec - start_time.tv_sec) * 1000000000) + (end_time.tv_nsec - start_time.tv_nsec); //Convert to nanoseconds
  long long total_wait_time = 0;
  for(int i = 0; i < num_threads; i++) {
    total_wait_time += thread_wait_times[i];
  }

  long long num_ops = (long long)(num_threads * num_iterations * 3);
  long long avg_time_per_op = time_difference / num_ops;
  long long avg_wait_for_lock = total_wait_time / num_ops;

  //Print results
  int test_name_length;
  
  if(!opt_yield) { strcat(test_name, "none"); }
  
  strcat(test_name,"-");
  
  switch(opt_sync) 
    {
    case 'm':
    case 's':
      test_name_length = strlen(test_name);
      test_name[test_name_length] = opt_sync;
      test_name[test_name_length + 1] = '\0';
      break;
    case 'd':
      strcat(test_name, "none");
      break;
    }

  if(opt_sync == 'm' || opt_sync == 's')
    printf("%s,%d,%d,%d,%lld,%lld,%lld,%lld\n",test_name, num_threads, num_iterations, num_lists, num_ops, time_difference, avg_time_per_op, avg_wait_for_lock);
  else
    printf("%s,%d,%d,%d,%lld,%lld,%lld\n",test_name, num_threads, num_iterations, num_lists, num_ops, time_difference, avg_time_per_op);
  
  
  //Free malloc-ed memory from heap
  free(threads);
  free(elements);
  free(tid);
  free(sub_lists);
  free(mutex_locks);
  free(spin_locks);
  free(thread_wait_times);
  

  exit(0);
}

//-------FUNCTION IMPLEMENTATIONS--------//

void *thread_function(void *arg) {
  int tid =  *(int *)arg;
  struct timespec start_time;
  struct timespec end_time;
  
  //1. Starts with a set of pre-allocated and initialized elements(--iterations=#)
  //2. Inserts them all into the multi-list (which sublist the key should go into determined by a hash of the key)
  for(int i = tid; i < (num_iterations * num_threads); i += num_threads) {
    if(opt_sync == 'm') {
      //Record time spent waiting for locks
      clock_gettime(CLOCK_MONOTONIC, &start_time);
      pthread_mutex_lock(&mutex_locks[offset[i]]);
      clock_gettime(CLOCK_MONOTONIC, &end_time);
      
      //Insert
      SortedList_insert(&sub_lists[offset[i]], &elements[i]); 
      pthread_mutex_unlock(&mutex_locks[offset[i]]);

      //Compute elapsed time difference, add to (per-thread) total
      thread_wait_times[tid] = ((end_time.tv_sec - start_time.tv_sec) * 1000000000) + (end_time.tv_nsec - start_time.tv_nsec);
    }
    else if(opt_sync == 's') {
      clock_gettime(CLOCK_MONOTONIC, &start_time);
      while(__sync_lock_test_and_set(&spin_locks[offset[i]], 1));
      clock_gettime(CLOCK_MONOTONIC, &end_time);
      //Insert
      SortedList_insert(&sub_lists[offset[i]], &elements[i]);
      __sync_lock_release(&spin_locks[offset[i]]);

      //Compute elapsed time difference, add to (per-thread) total
      thread_wait_times[tid] = ((end_time.tv_sec - start_time.tv_sec) * 1000000000) + (end_time.tv_nsec - start_time.tv_nsec);
    }
    else
      //Insert
      SortedList_insert(&sub_lists[offset[i]], &elements[i]);
  }

  //3. Gets the list length
  list_len = 0;

  if(opt_sync == 'm'){
    for(int i = 0; i < num_lists; i++) {
      //Record time spent waiting for locks
      clock_gettime(CLOCK_MONOTONIC, &start_time);
      pthread_mutex_lock(&mutex_locks[i]);
      clock_gettime(CLOCK_MONOTONIC, &end_time);

      //Length
      list_len += SortedList_length(&sub_lists[i]);
      pthread_mutex_unlock(&mutex_locks[i]);
      
      //Compute elapsed difference, add to (per-thread) total
      thread_wait_times[tid] += ((end_time.tv_sec - start_time.tv_sec) * 1000000000) + (end_time.tv_nsec - start_time.tv_nsec);
    }
  }
  else if(opt_sync == 's'){
    for(int i = 0; i < num_lists; i++) {
      clock_gettime(CLOCK_MONOTONIC, &start_time);
      while(__sync_lock_test_and_set(&spin_locks[i], 1) == 1);
      clock_gettime(CLOCK_MONOTONIC, &end_time);
      
      //Length
      list_len += SortedList_length(&sub_lists[i]);
      __sync_lock_release(&spin_locks[i]);

      //Compute elapsed time difference, add to (per-thread) total
      thread_wait_times[tid] = ((end_time.tv_sec - start_time.tv_sec) * 1000000000) + (end_time.tv_nsec - start_time.tv_nsec);
    }
  }
  else {
    for(int i = 0; i < num_lists; i++) {
      //Length
      list_len += SortedList_length(&sub_lists[i]);
    }
  }

  //4. Looks up and deletes each of the keys it inserted
  SortedListElement_t* delete_element;
  for(int i = tid; i < (num_iterations * num_threads); i += num_threads) {
    if(opt_sync == 'm') {
      //Record time spent waiting for locks
      clock_gettime(CLOCK_MONOTONIC, &start_time);
      pthread_mutex_lock(&mutex_locks[offset[i]]);
      clock_gettime(CLOCK_MONOTONIC, &end_time);

      //Delete
      delete_element = SortedList_lookup(&sub_lists[offset[i]], elements[i].key);
      SortedList_delete(delete_element);
      pthread_mutex_unlock(&mutex_locks[offset[i]]);
      
      //Compute elapsed time difference, add to (per-thread) total
      thread_wait_times[tid] += ((end_time.tv_sec - start_time.tv_sec) * 1000000000) + (end_time.tv_nsec - start_time.tv_nsec);
    }
    else if(opt_sync == 's') {
      clock_gettime(CLOCK_MONOTONIC, &start_time);
      while(__sync_lock_test_and_set(&spin_locks[offset[i]], 1) == 1);
      clock_gettime(CLOCK_MONOTONIC, &end_time);

      //Delete
      delete_element = SortedList_lookup(&sub_lists[offset[i]], elements[i].key);
      SortedList_delete(delete_element);
      __sync_lock_release(&spin_locks[offset[i]]);

      //Compute elapsed time difference, add to (per-thread) total
      thread_wait_times[tid] = ((end_time.tv_sec - start_time.tv_sec) * 1000000000) + (end_time.tv_nsec - start_time.tv_nsec);
    }
    else {
      //Delete
      delete_element = SortedList_lookup(&sub_lists[offset[i]], elements[i].key);
      SortedList_delete(delete_element);
    }
  }

  //5. Exits to re-join the parent thread
  return NULL;
}

