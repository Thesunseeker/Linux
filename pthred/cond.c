#include <stdio.h>
#include <unistd.h>
#include <pthread.h>


int is_have_noodles = 0;
pthread_mutex_t mutex;
pthread_cond_t hall_foodie;
pthread_cond_t kitchen_chef;

void *thr_chef()
{
  while(1)
  {
    pthread_mutex_lock(&mutex);
    while(is_have_noodles == 1)
    {
      pthread_cond_wait(&kitchen_chef , &mutex);
    }
    printf("made a bowl of noodles\n");
    is_have_noodles = 1;
    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&hall_foodie);
  }
  return NULL;
}

void *thr_foodie()
{
  while(1)
  {
    pthread_mutex_lock(&mutex);
    while(is_have_noodles == 0)
    {
      pthread_cond_wait(&hall_foodie , &mutex);
    }
    printf("delicous\n");
    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&kitchen_chef);
  }
}

int main()
{
  int ret , i;
  pthread_t ctid[4] , etid[4];
  
  pthread_mutex_init(&mutex , NULL);
  pthread_cond_init(&hall_foodie , NULL);
  pthread_cond_init(&kitchen_chef , NULL);
  for(int i = 0 ; i < 4 ; i++)
  {
    ret = pthread_create(&ctid[i] , NULL , thr_chef ,(void*)i);
    if(ret != 0)
    {
      printf("create chef error\n");
      return -1;
    }
  }
  for(i = 0 ; i < 4 ; i++)
  {
    ret = pthread_create(&etid[i] , NULL , thr_foodie ,(void*)i);
    if(ret != 0)
    {
      printf("create foodie error\n");
      return -1;
    }
  }
  for(int i = 0 ; i < 4 ; i ++)
  {
   pthread_join(ctid[i] , NULL);
   pthread_join(etid[i] , NULL);
  }
  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&hall_foodie);
  pthread_cond_destroy(&kitchen_chef);
  return 0;
}
