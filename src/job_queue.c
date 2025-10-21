#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <sys/_pthread/_pthread_cond_t.h>
#include <sys/_pthread/_pthread_mutex_t.h>

#include "job_queue.h"

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int die = 0;

int job_queue_init(struct job_queue *job_queue, int capacity) {
  int exit_code = -1;
  // Initialize capacity and allocate memory for actual queue
  job_queue->current_capacity= capacity;
  job_queue->total_capacity = capacity;
  job_queue->queue = malloc(sizeof(char)*capacity*100);
  job_queue->next_job = 0;
  job_queue->insert_point = 0;
  job_queue->jobs_in_queue = 0;
  // Check for malloc error
  if (job_queue->queue != NULL) exit_code = 0;

  return exit_code;
}

int job_queue_destroy(struct job_queue *job_queue) {
  int exit_code = -1;

  while (job_queue->current_capacity != job_queue->total_capacity) {
    pthread_mutex_lock(&lock);
    pthread_cond_wait(&cond, &lock);
    pthread_mutex_unlock(&lock);
  }
  die = 1;
  pthread_cond_broadcast(&cond);
  free(job_queue->queue);
  exit_code = 0;

  return exit_code;
}

int job_queue_push(struct job_queue *job_queue, void *data) {
  int exit_code = -1;
  // printf("start push\n");
  pthread_mutex_lock(&lock);
  while (job_queue->current_capacity <= 0) {
    
    pthread_cond_wait(&cond, &lock);

  }
  // Insert data at next available point
  job_queue->queue[job_queue->insert_point] = data;
  
  // Decrement current_capacity and advance next available position
  // Circular for now, as I guess that is what's intended
  job_queue->current_capacity--;
  job_queue->jobs_in_queue++;
  job_queue->insert_point++;
  job_queue->insert_point = job_queue->insert_point%job_queue->total_capacity;
  pthread_cond_broadcast(&cond);
  pthread_mutex_unlock(&lock);
  exit_code = 0;

  return exit_code;
}

int job_queue_pop(struct job_queue *job_queue, void **data) {
  int exit_code = -1;

  pthread_mutex_lock(&lock);
  while (job_queue->current_capacity == job_queue->total_capacity && !die) {
    // If queue is empty, wait until job has been pushed
    // Consider if this condition is robust enough, or if queue should
    // have a current_size member.
    
    pthread_cond_wait(&cond, &lock);
    
  }
  // If queue is to be killed, wake up everybody and return.
  if (die) {
    pthread_mutex_unlock(&lock);
    pthread_cond_broadcast(&cond);
    return exit_code;
  }


    // Place next available job in output location.
    // Should be pointer to void pointer, but still need
    // to figure out how that works.
    data[0] = (void*) (job_queue->queue[job_queue->next_job]);
    job_queue->queue[job_queue->next_job] = NULL;
    job_queue->next_job++;
    job_queue->next_job = job_queue->next_job%job_queue->total_capacity;
    job_queue->current_capacity++;
    job_queue->jobs_in_queue--;
    exit_code = 0;
    pthread_mutex_unlock(&lock);
    pthread_cond_signal(&cond);

  return exit_code;
}
