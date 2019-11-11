#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

int ticket = 100;
pthread_mutex_t  mutex;

void *ticket_scalper(void *arg)
{
  char* id = (char*)arg;
  while(1)
  {
    //互斥锁保护的是临界资源，加锁放在临界操作之前
    //int pthread_mutex_lock(pthread_mutex_t *mutex);
    //阻塞加锁--无法加则挂起等待
    //int pthread_mutex_trylock(pthread_mutex_t *mutex);
    //非阻塞加锁--无法加锁则立即报错返回
    pthread_mutex_lock(&mutex);
    if(ticket > 0)
    {
      printf("%sticket_scalper:---get a ticket:%d\n", id ,ticket);
      ticket--;
    }
    else{
      //需要在线程任意有可能推出的地方进行解锁
      pthread_mutex_unlock(&mutex);
      pthread_exit(NULL);
    }
    //临界资源操作完毕后一定记得解锁
    pthread_mutex_unlock(&mutex);
  }
  return NULL;
}

int main()
{
  int i = 0, ret;
  pthread_t tid[4];
   //互斥锁初始化
   //int pthread_mutex_init(pthread_mutex_t *mutex,
   //  const pthread_mutexattr_t *restrict attr);
   //  mutex:互斥锁变量
   //  attr：互斥锁属性(通常置NULL)
   pthread_mutex_init(&mutex , NULL);
   for(i = 0 ; i < 4 ; i++)
   {
     ret = pthread_create(&tid[i] , NULL , ticket_scalper , (void*)i);
     if(ret != 0)
     {
      printf("create scalper failed !!\n");
      return -1;
     }
   }
   for(i = 0 ; i < 4 ; i++)
   {
     pthread_join(tid[i] , NULL);
   }
    //销毁互斥锁，释放资源
    //int pthread_mutex_destroy(pthread_mutex_t *mutex)
    pthread_mutex_destroy(&mutex);
    return 0;
}

