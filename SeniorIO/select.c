/*演示select的最基本使用 , 对标准输入进行可读事件监控 , 就绪只有对其进行操作*/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>


int main()
{
  fd_set set;
  FD_ZERO(&set);//清空集合
  FD_SET(0 , &set);//将标准输入添加到监控
  int maxfd = 0;
  while(1)
  {
    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    FD_SET(0 , &set);//每次都要重新添加所有描述符
    int nfds = select(maxfd + 1 , &set , NULL , NULL ,&tv);
    if(nfds < 0)
    {
      printf("select error\n");
      return  -1;
    }
    else if (nfds == 0)
    {
      printf("wait timeout\n");
      continue;
    }
    printf("input-------\n");
    //select返回的是就绪集合
    int i;
    for(i = 0 ; i <= maxfd ; i++)
    {
      char buf[1024] = {0};
      int ret = read(i ,buf , 1023);
      if(ret < 0)
      {
        printf("read error\n");
        return -1;
      }
      printf("get buf:[%s]\n", buf);
    }
  }
  return 0;
}
