#include <unistd.h>
#include <queue>
#include <vector>
#include <time.h>
#include <pthread.h>
#include <stdlib.h>
#include <thread>
#include <sstream>
#include <iostream>

#define MAX_THREAD 5
#define MAX_QUEUE 10
typedef void (*handler_t)(int);

class ThreadTask
{
  private:
    int _data;
    handler_t _handler;
  public:
    ThreadTask(int data, handler_t handle)
      :_data(data)
      ,_handler(handle)
     {}
    
    void SetTask(int data , handler_t handle)//设置任务
    {
      _data = data;
      _handler = handle;
      return;
    }

    void TaskRun()
    {
      return _handler(_data);//用户自己传入方法去处理数据
    }
};

class ThreadPool
{
  private:
    std::queue<ThreadTask> _queue;
    int _capacity;
    pthread_mutex_t _mutex;
    pthread_cond_t _cond_pro;//生产者等待队列
    pthread_cond_t _cond_con;//消费者等待队列

    int _thr_max;//控制线程的最大数量
  
//  public:
//    ThreadPool();
//    ThreadInit();//创建出指定数量的线程
//    TaskPush(MyTask &t);//线程安全的入队
//    TaskPop(MyTask &t);//线程安全的任务出队
//    thr_start()
//    1.线程安全的任务出队
//        若没有任务退出且标记为真,则退出线程
//    2.处理任务
//    Threadstop()  将退出标志标为真,判断线程数量是否>0
  public:
    ThreadPool(int maxq = MAX_QUEUE , int maxt = MAX_THREAD)
    :_capacity(maxq)
     ,_thr_max(maxt)
    {
      pthread_mutex_init(&_mutex , NULL);
      pthread_cond_init(&_cond_con , NULL);
      pthread_cond_init(&_cond_pro , NULL);
    }
    
    ~ThreadPool()
    {
      pthread_mutex_destroy(&_mutex);
      pthread_cond_destroy(&_cond_con);
      pthread_cond_destroy(&_cond_pro);
    }

    bool PoolInit()
    {
      for(int i = 0 ; i < _thr_max ; i++)
      {
        std::thread thr(&ThreadPool::thr_start , this);//声明哪一个类中的函数,取了成员函数后
        //需将成员函数的this指针传给他
        //phtread_cread(tid , NULL , thr_start , this)
        thr.detach();
      }
      return  true;
    }

    bool TaskPush(ThreadTask &tt)
    {
      pthread_mutex_lock(&_mutex);
      while(_queue.size() == _capacity)
      {
        pthread_cond_wait(&_cond_pro , &_mutex);
      }
      _queue.push(tt);
      pthread_mutex_unlock(&_mutex);
      pthread_cond_signal(&_cond_con);
      return true;
    }


  private:
    void thr_start()//出队
    {
      while(1)
      {
        pthread_mutex_lock(&_mutex);
        while(_queue.empty())
        {
          pthread_cond_wait(&_cond_con , &_mutex);
        }
        ThreadTask tt = _queue.front();
        _queue.pop();
        pthread_mutex_unlock(&_mutex);
        pthread_cond_signal(&_cond_pro);
        //任务处理应该放在解锁后,否则会造成同一时间只有一个线程在处理任务
        tt.TaskRun();
      }
    }    
};


