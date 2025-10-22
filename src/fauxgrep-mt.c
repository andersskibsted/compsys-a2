// Setting _DEFAULT_SOURCE is necessary to activate visibility of
// certain header file contents on GNU/Linux systems.
#include <sys/_pthread/_pthread_cond_t.h>
#include <sys/_pthread/_pthread_mutex_t.h>
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fts.h>

// err.h contains various nonstandard BSD extensions, but they are
// very handy.
#include <err.h>

#include <pthread.h>

#include "job_queue.h"

pthread_mutex_t fglock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t fgcond = PTHREAD_COND_INITIALIZER;

typedef struct __thread_args {
  const char *needle;
  struct job_queue *jq;
} thread_args;

int fauxgrep_file(char const *needle, char const *path) {
  FILE *f = fopen(path, "r");

  if (f == NULL) {
    warn("failed to open %s", path);
    return -1;
  }

  char *line = NULL;
  size_t linelen = 0;
  int lineno = 1;

  while (getline(&line, &linelen, f) != -1) {
    if (strstr(line, needle) != NULL) {
      printf("%s:%d: %s", path, lineno, line);
    }

    lineno++;
  }

  free(line);
  fclose(f);

  return 0;
}

void *worker(void *arg) {
  thread_args* args = (thread_args*) arg;
  struct job_queue *jq = args->jq;
  const char* needle = args->needle;

  while (1) {
    char *path;
    pthread_mutex_lock(&fglock);
    if (job_queue_pop(jq, (void **)&path) == 0) { 
      // Process next job-string in queue.
      fauxgrep_file(needle, path);
      free(path);
      pthread_mutex_unlock(&fglock);
    } else {
      // Job queue empty, end thread.
      pthread_mutex_unlock(&fglock);
      break;
    }
  }
  return NULL;
}


int main(int argc, char * const *argv) {
  if (argc < 2) {
    err(1, "usage: [-n INT] STRING paths...");
    exit(1);
  }

  int num_threads = 1;
  char const *needle = argv[1];
  char * const *paths = &argv[2];


  if (argc > 3 && strcmp(argv[1], "-n") == 0) {
    // Since atoi() simply returns zero on syntax errors, we cannot
    // distinguish between the user entering a zero, or some
    // non-numeric garbage.  In fact, we cannot even tell whether the
    // given option is suffixed by garbage, i.e. '123foo' returns
    // '123'.  A more robust solution would use strtol(), but its
    // interface is more complicated, so here we are.
    num_threads = atoi(argv[2]);

    if (num_threads < 1) {
      err(1, "invalid thread count: %s", argv[2]);
    }

    needle = argv[3];
    paths = &argv[4];

  } else {
    needle = argv[1];
    paths = &argv[2];
  }

  struct job_queue queue;
  job_queue_init(&queue, 64);

  // Create worker threads and bundle needle in with queue as argument.
  // This way the workers will know the needle, and can just pop the
  // path from job_queue.   
  pthread_t *threads = calloc(num_threads, sizeof(pthread_t));
  thread_args *thread_arg = malloc(sizeof(thread_args));
  thread_arg->jq = &queue;
  thread_arg->needle = needle;
  for (int i = 0; i<num_threads; i++) {
    pthread_create(&threads[i], NULL, &worker, thread_arg);
  }  
  // 
  // FTS_LOGICAL = follow symbolic links
  // FTS_NOCHDIR = do not change the working directory of the process
  //
  // (These are not particularly important distinctions for our simple
  // uses.)
  int fts_options = FTS_LOGICAL | FTS_NOCHDIR;
  

  FTS *ftsp;
  if ((ftsp = fts_open(paths, fts_options, NULL)) == NULL) {
    err(1, "fts_open() failed");
    return -1;
  }

  FTSENT *p;
  while ((p = fts_read(ftsp)) != NULL) {
    
    switch (p->fts_info) {
    case FTS_D:
      break;
    case FTS_F:
      // Process the file p->fts_path, somehow.
      job_queue_push(&queue, (void *)strdup(p->fts_path));
      break;
    default:
      break;
    }
  }
  fts_close(ftsp);
  
  job_queue_destroy(&queue); // Shut down the job queue and the worker threads here.

  // Wait for all threads to finish before exiting.
  for (int i = 0; i < num_threads; i++) {
    if (pthread_join(threads[i], NULL) != 0) {
      err(1, "pthread_join() failed");
    }
  }    
  free(threads);
  return 0;
}
