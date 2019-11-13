#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

void *thr_start(void *arg) 
{
 char buf[] = "nihao";
 //   char *buf = "nihao";
    sleep(5);
    return buf;
 
}
int main()
{
pthread_t tid;
    int ret = pthread_create(&tid, NULL, thr_start, NULL);
    if (ret != 0) {
          printf("create thread error\n");
          return -1;
                          
    }
    pthread_detach(tid);
    void *retval;
    int err = pthread_join(tid, &retval);
    if (err == EINVAL) {
      printf("thread is not a joinable thread\n");
      return -1;
    }
    printf("thread exit val:%s\n", (char*)retval);
    return 0;
}


