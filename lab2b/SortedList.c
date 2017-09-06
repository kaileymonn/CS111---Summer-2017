//NAME: Kai Wong
//EMAIL: kaileymon@g.ucla.edu
//ID: 704451679

/*
 * SortedList (and SortedListElement)
 *
 *A doubly linked list, kept sorted by a specified key.
 *This structure is used for a list head, and each element
 *of the list begins with this structure.
 *
 *The list head is in the list, and an empty list contains
 *only a list head.  The list head is also recognizable because
 *it has a NULL key pointer.
 */

#include <stdio.h>
#include <string.h>
#include <sched.h>
#include "SortedList.h"

//SortedList_insert ... insert an element into a sorted list
void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {
  if(list == NULL || element == NULL) { return; }
  
  SortedListElement_t *current = list->next;
  
  //Iterate to node at next position
  while(current != list) {
    if(strcmp(element->key, current->key) <= 0) 
      break;
    current = current->next;
  }
  
  if(opt_yield & INSERT_YIELD)
    sched_yield();
  
  //Insertion
  element->next = current;
  element->prev = current->prev;
  current->prev->next = element;
  current->prev = element;
}


//SortedList_delete ... remove an element from a sorted list
int SortedList_delete( SortedListElement_t *element) {
  if(element == NULL) { return 1; }
  
  //Check thaat next->prev and prev->next both point to element node
  if(element->next->prev == element->prev->next) {
   
    if(opt_yield & DELETE_YIELD)
      sched_yield();
    
    //Deletion
    element->prev->next = element->next;
    element->next->prev = element->prev;
    return 0;
  }
  return 1;
}


// SortedList_lookup ... search sorted list for a key
SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
  if(list == NULL || key == NULL) { return NULL; }
  
  SortedListElement_t *current = list->next;
  
  //Lookup
  while(current != list) {
    if(strcmp(current->key, key) == 0) { return current;}
    
    if(opt_yield & LOOKUP_YIELD)
      sched_yield();
    
    current = current->next;
  }

  //Not found
  return NULL;
}


//SortedList_length ... count elements in a sorted list
int SortedList_length(SortedList_t *list) {
  int counter = 0;
  
  if(list == NULL) { return -1; }
  
  SortedListElement_t *current = list->next;
  
  //Iterate through
  while(current != list) {
    counter++;
    
    if(opt_yield & LOOKUP_YIELD)
      sched_yield();
    
    current = current->next;
  }
  
  return counter;
}

