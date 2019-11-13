/*
*   文件名称： 
*   *   创 建 
*   *   创建日期：
*   *   描    述：使用信号量实现生产者与消费者模型 , 唤醒队列 
*   ================================================================*/
#include <iostream>
#include <vector>
#include <thread>
#include <semaphore.h>

#define MAXQ 10

class RingQueue
{
  public:
    RingQueue(int maxq = MAXQ)
      :_queue(maxq)
      ,_capacity(maxq)
  {
    sem_init(&_lock , 0 , 1);//第二个参数0/1表示用于线程间或进程间,0为线程
    sem_init(&_idle_space , 0 , maxq);
    sem_init(&_data_space, 0 , 0);
  }
    ~RingQueue()
    {
      sem_destroy(&_lock);
      sem_destroy(&_data_space);
      sem_destroy(&_idle_space);
    }
    bool QueuePush(int data)
    {
      sem_wait(&_idle_space);//判断队列是否满了
      sem_wait(&_lock);//计数-1 加锁
      _queue[_step_write] = data;
      _step_write = (_step_write + 1) % _capacity;//写指针向后移动
      sem_post(&_lock);//进行计数+1 解锁
      sem_post(&_data_space);//数据空间+1
      return true;
    }
    bool QueuePop(int &data)
    {
      sem_wait(&_lock);
      sem_wait(&_data_space);
      data = _queue[_step_read];
      _step_read = (_step_read + 1) % _capacity;
      sem_post(&_lock);
      sem_post(&_idle_space);//空闲空间多一个 唤醒生产者
      return true;
    }

  private:
    std::vector<int> _queue;
    int _capacity;//节点限制
    int _step_read;//读指针
    int _step_write;//写指针
    sem_t _lock;
    sem_t _idle_space;//空闲空间计数
    sem_t _data_space;//数据空间计数
};

void thr_producer(RingQueue *q)
{
  int data = 0;
  while(1)
  {
    q->QueuePush(data);
    std::cout << "put data------" << data++ << std::endl;
  }
  return;
}

void thr_consumer(RingQueue *q)
{
  int data = 0 ;
  while(1)
  {
    q->QueuePop(data);
    std::cout << "get data--"<< data << std::endl;
  }
  return;
}

int main()
{
  RingQueue q;
  std::vector<std::thread> list_con(4);
  std::vector<std::thread> list_pro(4);
  for(int i = 0 ; i < 4 ; i++)
  {
    list_pro[i] = std::thread (thr_producer , &q ,std::ref(q));
  }
  for(int i = 0 ; i < 4 ; i++)
  {
    list_con[i] = std::thread(thr_consumer , &q);
  }
  for(int i = 0 ; i < 4 ; i++)
  {
    list_con[i].join();
    list_pro[i].join();
  }
  return 0 ;
}
