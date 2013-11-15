#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>


struct whatever_s {
  char letter;
  int count;
  int chunksize;
  unsigned int microseconds;
  pthread_mutex_t mutex;
};


void whatever_s_init(struct whatever_s * ws,
		     char letter, int count,
		     int chunksize, unsigned int microseconds);
void whatever_s_destroy(struct whatever_s * ws);
void * whatever_thread(struct whatever_s * ws);
pthread_t run_whatever(char letter, int count,
		       int chunksize, unsigned int microseconds);


int main(int argc, char ** argv)
{
  pthread_t thread[3];
  struct whatever_s * whatever;
  int i;
  thread[0] = run_whatever('o', 200,  1, 12500);
  thread[1] = run_whatever('x', 200,  1, 12500);
  thread[2] = run_whatever('-',  50, 10, 50000);
  
  for(i = 0; i < 3; ++i)
    if(0 == thread[i])
      fprintf(stderr, "thread[%d] == 0\n", i);
    else if(0 != pthread_join(thread[i], (void **) & whatever))
      fprintf(stderr, "join(thread[%d]) failed: %s\n", i, strerror(errno));
    else if(0 == whatever)
      fprintf(stderr, "error in thread %d.\n", i);
    else{
      fprintf(stderr, "joined '%c'\n", whatever->letter);
      whatever_s_destroy(whatever);
      free(whatever);
    }
  
  return 0;
}


void * whatever_thread(struct whatever_s * ws)
{
  while(ws->count > 0){
    int i;

    if(0 != pthread_mutex_lock(& ws->mutex)){
      whatever_s_destroy(ws);
      free(ws);
      perror("pthread_mutex_lock");
      return 0;
    }
    
    for(i = 0; i < ws->chunksize; ++i)
      fprintf(stderr, "%c", ws->letter);

    if(0 != pthread_mutex_unlock(& ws->mutex)){
      whatever_s_destroy(ws);
      free(ws);
      perror("pthread_mutex_unlock");
      return 0;
    }
    
    ws->count -= ws->chunksize;
    usleep(ws->microseconds);
  }
  fprintf(stderr, "%c", ws->letter);
  
  return ws;
}


pthread_t run_whatever(char letter,
		       int count,
		       int chunksize,
		       unsigned int microseconds)
{
  struct whatever_s * ws = malloc(sizeof(* ws));
  pthread_t result;
  
  if(0 == ws)
    return 0;
  whatever_s_init(ws, letter, count, chunksize, microseconds);
  
  if(0 != pthread_create(& result, 0, (void*(*)(void*)) whatever_thread, ws)){
    whatever_s_destroy(ws);
    free(ws);
    perror("pthread_create");
    exit(EXIT_FAILURE);
  }
  
  return result;
}


void whatever_s_init(struct whatever_s * ws,
		     char letter,
		     int count,
		     int chunksize,
		     unsigned int microseconds)
{
  ws->letter = letter;
  ws->count = count;
  ws->chunksize = chunksize;
  ws->microseconds = microseconds;
  if(0 != pthread_mutex_init(& ws->mutex, 0)){
    perror("pthread_mutex_init");
    exit(EXIT_FAILURE);
  }
}


void whatever_s_destroy(struct whatever_s * ws)
{
  if(0 != pthread_mutex_destroy(& ws->mutex))
    perror("WARNING pthread_mutex_destroy");
}
