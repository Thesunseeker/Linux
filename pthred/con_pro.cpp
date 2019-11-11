/*=============================================================== 
*   Copyright (C) . All rights reserved.")
*   文件名称： 
*   创 建 者：zhang
*   创建日期：
*   描    述：基于互斥锁与条件变量实现一个线程安全的队列
*       实现生产者与消费者模型 
 ================================================================*/

#include <iostream>
#include <pthread.h>
#include <queue>
#define MAXQ 10

class BlockQueue{
private:
  std::queue<int> _queue;
  int _capacity;
  pthread_mutex_t _mutex;
  pthread_cond_t _cond_productor;
  pthread_cond_t _cond_consumer;
public:
  BlockQueue(int maxq = MAXQ)
    :_capacity(maxq)
  {
    pthread_mutex_init(&_mutex , NULL);
    pthread_cond_init(&_cond_productor , NULL);
    pthread_cond_init(&_cond_consumer , NULL);
  }
  
  ~BlockQueue()
  {
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_cond_productor);
    pthread_cond_destroy(&_cond_consumer);
  }

//  bool IsFull()
// {
//    return (_queue.size() == _capacity ? true : false);
//  }
  
  bool QueuePush(int data)
  {
    pthread_mutex_lock(&_mutex);
    while(_queue.size() == _capacity)
    {
      pthread_cond_wait(&_cond_productor , &_mutex);
    }
    _queue.push(data);
    pthread_mutex_unlock(&_mutex);
    pthread_cond_signal(&_cond_consumer);
    return true;
  }
  
  bool QueuePop(int &data)
  {
    pthread_mutex_lock(&_mutex);
    while(_queue.empty())
    {
      pthread_cond_wait(&_cond_consumer , &_mutex);
    }
    data = _queue.front();
    _queue.pop();
    pthread_mutex_unlock(&_mutex);
    pthread_cond_signal(&_cond_productor);
    
    return true;
  }
};

void *thr_consumer(void *arg)
{
  BlockQueue *q = (BlockQueue*)arg;
  int data;
  while(1)
  {
    q->QueuePop(data);
    std::cout << "consumer get a piece of data --"<< data << "\n";
  }
}

void *thr_productor(void *arg)
{
  BlockQueue *q = (BlockQueue*)arg;
  int data = 0;
  while(1)
  {
    q->QueuePush(data);
    std::cout << "productor produces a data" << data ++ << "\n";
  }
  return NULL;
}

#define MAXTHR 1
int main()
{
  pthread_t ctid[MAXTHR] , ptid[MAXTHR];
  int ret , i;

  BlockQueue q;
  for(i = 0 ; i <MAXTHR ; i++)
  {
    ret = pthread_create( &ctid[i] , NULL , thr_consumer , (void*)&q);
    if(ret != 0 )
    {
      std::cout << "creadte thread error \n";
      return -1;
    }
  }
  for(i = 0 ; i < MAXTHR ; i++)
  {
    ret = pthread_create(&ptid[i] , NULL , thr_productor , (void*)&q);
    if(ret != 0)
    {
      std::cout << "creat thread error \n";
      return -1;
    }
  }
  for(i = 0 ;i < MAXTHR ; i++)
  {
    pthread_join(ctid[i] , NULL);
    pthread_join(ptid[i] , NULL);
  }
  return 0;
}
