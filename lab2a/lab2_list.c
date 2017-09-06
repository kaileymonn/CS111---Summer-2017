//NAME: Kai Wong
//EMAIL: kaileymon@g.ucla.edu
//ID: 704451679

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include "SortedList.h"

//----------------GLOBALS-----------------//
int num_threads = 1;
int num_iterations = 1;

int lock = 0;
int opt_yield = 0;
char opt_sync = 'd';
char test_name[15] = "list-";

long long counter = 0;
pthread_mutex_t mutex;

struct timespec start, end;

SortedList_t* list;
SortedListElement_t* elements; 

char *char_array = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

//-----------ADDITIONAL FUNCTIONS---------//

//Thread routine
void *t_function(void *arg);

//Formating of stdout for .csv file
void print_csv();


//-------------MAIN ROUTINE--------------//

int main(int argc, char**argv) {
  int ch;
  
  static struct option long_opts[] = {
    {"threads", required_argument, 0, 't'},
    {"iterations", required_argument, 0, 'i'},
    {"sync", required_argument, 0, 's'},
    {"yield", required_argument, 0, 'y'},
  };
  
  
  while((ch = getopt_long(argc, argv, "t:i:s:y:", long_opts, NULL)) != -1) {
    switch(ch) 
      {
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

  print_csv();

  //Initialize DLL 
  list = malloc(sizeof(SortedList_t));
  list->key = NULL;
  list->next = list;
  list->prev = list;

  //Initializations for threading
  int num_elems = num_iterations * num_threads;
  elements = malloc(sizeof(SortedListElement_t) * num_elems);
  pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);
  int* tid = malloc(num_threads * sizeof(int));

  srand(time(NULL));

  //Suboptimal routine
  for(int i = 0; i < num_elems; i++) {
    int key_len = rand() % 5 + 10;
    char *key = malloc((key_len + 1) * sizeof(char));
    
    int j;
    for(j = 0; j < key_len; j++) {
      key[j] = char_array[rand() % strlen(char_array)];
    }
    key[j] = '\0';
    elements[i].key = key;
  }

  if (opt_sync == 'm') { pthread_mutex_init(&mutex, NULL); }
  
  clock_gettime(CLOCK_MONOTONIC, &start);
  
  for (int i = 0; i < num_threads; i++) {
    tid[i] = i;
    if(pthread_create(&(threads[i]), NULL, t_function, &tid[i]) < 0) {
      perror("Failed to create pthreads"); 
      exit(1);
    }
  }

  for(int i = 0; i < num_threads; i++) {
    if(pthread_join(threads[i], NULL) < 0) {
      perror("Failed to join pthreads"); 
      exit(1);
    }
  }

  clock_gettime(CLOCK_MONOTONIC, &end);

  //Print info
  long long total_time = (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);
  int num_ops = num_threads * num_iterations * 3;
  long long avg = total_time / num_ops;
  

  int list_len = SortedList_length(list);
  
  if(list_len != 0) {
    fprintf(stderr, "Invalid list length, length = %d\n", list_len);
    exit(2);
  }

  //TODO
  printf("%s,%d,%d,%d,%d,%lld,%lld\n",test_name, num_threads, num_iterations, 1, num_ops, total_time, avg);
  
  //Free malloc-ed memory from heap
  free(threads);
  free(elements);
  free(tid);

  exit(0);
}

//-------FUNCTION IMPLEMENTATIONS--------//

void *t_function(void *arg) {
  int tid = *(int *)arg;
  
  for(int i = tid; i < num_iterations * num_threads; i += num_threads) {
    
    if(opt_sync == 'm') {
      pthread_mutex_lock(&mutex);
      SortedList_insert(list, &elements[i]);
      pthread_mutex_unlock(&mutex);
    }
  
    else if(opt_sync == 's') {
      while(__sync_lock_test_and_set(&lock, 1) == 1);
      SortedList_insert(list, &elements[i]);
      __sync_lock_release(&lock);
    }
    
    else { SortedList_insert(list, &elements[i]); }
  }
  
  SortedList_length(list);
  SortedListElement_t* temp;

  for(int i = tid; i < num_iterations * num_threads; i += num_threads) {
  
    if(opt_sync == 'm') {
      
      pthread_mutex_lock(&mutex);
      temp = SortedList_lookup(list, elements[i].key);
      
      if(temp == NULL) {
	perror("Error: Key not found");
	exit(2);
      }
      
      SortedList_delete(temp);
      pthread_mutex_unlock(&mutex);
    }
    
    else if(opt_sync == 's') {
      while(__sync_lock_test_and_set(&lock, 1) == 1);
    
      temp = SortedList_lookup(list, elements[i].key);
    
      if(temp == NULL) {
	perror("Error: Key not found");
	exit(2);
      }
      
      SortedList_delete(temp);
      __sync_lock_release(&lock);
    }
    
    else {
      temp = SortedList_lookup(list, elements[i].key);
      
      if(temp == NULL) {
	perror("Error: Key not found");
	exit(2);
      }
      
      SortedList_delete(temp);
    }
  }
  
  return NULL;
}

void print_csv() {
  int t_len;
  
  if(!opt_yield) { strcat(test_name, "none"); }
  
  strcat(test_name,"-");
  
  switch(opt_sync) 
    {
    case 'm':
    case 's':
      t_len = strlen(test_name);
      test_name[t_len] = opt_sync;
      test_name[t_len + 1] = '\0';
      break;
    case 'd':
      strcat(test_name, "none");
      break;
    }
}
