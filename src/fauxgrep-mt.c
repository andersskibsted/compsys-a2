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

typedef struct __worker_args {
  const char *needle;
  const char *path;
} worker_args;  

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
  struct job_queue *jq = arg;

  while (1) {
    worker_args *args;
    if (job_queue_pop(jq, (void*)args) == 0) {
      // Process next job-string in queue.
      const char *new_needle = args->needle;
      const char *new_path = args->path;
      fauxgrep_file(new_needle, new_path);
    } else {
      // Job queue empty, end thread.
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

  //struct job_queue *queue = malloc(sizeof(struct job_queue));
  struct job_queue queue;
  job_queue_init(&queue, 64);

  // Create worker threads
  pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
  for (int i = 0; i<num_threads; i++) {
    pthread_create(&threads[i], NULL, &worker, &queue);
  }  
  // 
  // FTS_LOGICAL = follow symbolic links
  // FTS_NOCHDIR = do not change the working directory of the process
  //
  // (These are not particularly important distinctions for our simple
  // uses.)
  int fts_options = FTS_LOGICAL | FTS_NOCHDIR;
  
  // allocate space for worker_args. Probably should be an array, as
  // this seems to be asking for memory-trouble  
  /* worker_args **current_arg = malloc(sizeof(worker_args)*num_threads); */


  FTS *ftsp;
  if ((ftsp = fts_open(paths, fts_options, NULL)) == NULL) {
    err(1, "fts_open() failed");
    return -1;
  }

  FTSENT *p;
  while ((p = fts_read(ftsp)) != NULL) {
    worker_args *arg = malloc(sizeof(worker_args));
    
    switch (p->fts_info) {
    case FTS_D:
      break;
    case FTS_F: 
      arg->needle = needle;
      arg->path = p->fts_path;
      job_queue_push(&queue, arg);
      // Process the file p->fts_path, somehow.
        
      break;
    default:
      break;
    }
  }

  fts_close(ftsp);

  job_queue_destroy(&queue); // Shut down the job queue and the worker threads here.

  return 0;
}
