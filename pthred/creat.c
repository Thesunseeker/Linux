/*=============================================================== 
 * *   Copyright (C) . All rights reserved.")
 * *   文件名称： 
 * *   创 建 者：zhang
 * *   创建日期：
 * *   描    述：这个demo用于实现基本的线程创建，体会接口的使用
 * *   int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
 * *       void *(*start_routine) (void *), void *arg); 
 * ================================================================*/
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

void *thr_start(void *arg)
{
  pthread_t tid = pthread_self();
  printf("----ordirnary----%s-%p\n" , (char*)arg , tid);
  sleep(5);
  return NULL;
}

int main()
{
  int ret ;
  pthread_t tid;
  char tmp[] = "nihao";
  pthread_t mtid = pthread_self();
  ret = pthread_create(&tid , NULL ,thr_start , (void*)tmp);
  if(ret != 0)
  {
    printf("thread cread error\n");
    return -1;
  }
  pthread_cancel(tid);
  pthread_exit(NULL);
  while(1)
  {
    printf("---main---mtid:%p---ctid:%p\n",mtid , tid);
    sleep(1);
  }
  return 0;
}
