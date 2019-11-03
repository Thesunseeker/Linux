/*基于select实现一个并发服务器
 * 对Select进行封装 , 封装一个类实例化一个类*/
#include <stdlib.h>
#include <vector>
#include <string>
#include <sys/select.h>
#include <iostream>
#include <pthread.h>
#include "tcpsocket.hpp"

class Select
{
  public:
    Select()
      :_maxfd(-1)
    {
      FD_ZERO(&_rfds);
    }
    ~Select()
    {}
    bool Add(TcpSocket &sock)
    {
      int fd = sock.GetFd();
      FD_SET(fd , &_rfds);
      _maxfd = _maxfd > fd ? _maxfd : fd;
      return true;
    }
    bool Wait(std::vector<TcpSocket> &list , int sec = 3)
    {
      struct timeval tv;
      tv.tv_sec = sec;
      tv.tv_usec = 0;
      int count;
      //避免对象的监控集合被修改,避免每次重新向集合中添加描述符
      fd_set tmp_set = _rfds;//每次定义新的集合进行监控 
      count = select(_maxfd+1 , &tmp_set , NULL , NULL , &tv);
      if(count < 0)
      {
        std::cout<<"select error\n";
        return false;
      }
      else if (count == 0 )
      {
        std::cout<< "Wait timeout\n";
        return false;
      }
      for(int i = 0 ; i <= _maxfd ; i++)
      {
        if(FD_ISSET(i , &tmp_set))
        {
          TcpSocket sock;
          sock.SetFd(i);
          list.push_back(sock);
        }
      }
      return true;
    }
    bool Del(TcpSocket &sock)
    {
      int fd = sock.GetFd();
      FD_CLR(fd , &_rfds);
      for(int i = _maxfd ;i >=0; i++)
      {
        if(FD_ISSET(i , &_rfds))
        {
          _maxfd = i;
          return true;
        }
      }
      _maxfd = -1;
      return true;
    }
  private:
    fd_set _rfds;
    int _maxfd;
};

int main(int argc , char *argv[])
{
  if(argc != 3)
  {
    std::cout << "./tcp_select ip port\n";
    return -1;
  }
  std::string srv_ip = argv[1];
  uint16_t srv_port = atoi(argv[2]);
  TcpSocket sock;
  CHECK_RET(sock.Socket());
  CHECK_RET(sock.Bind(srv_ip , srv_port));
  CHECK_RET(sock.Listen());
  Select s;
  s.Add(sock);
  while(1)
  {
    std::vector<TcpSocket> list;
    if(s.Wait(list) == false)
    {
      continue;
    }
    for(auto clisock : list)
    {
      if(clisock.GetFd() == sock.GetFd())
      {
        //当前就绪的描述符是监听套接字,应accept
        TcpSocket clisock;
        if(sock.Accept(clisock) == false)
        {
          continue;
        }
        s.Add(clisock);
      }
      else
        {
          //当前就绪的描述符是通信套接字,应recv
          std::string buf;
          if(clisock.Recv(buf) == false)
          {
            s.Del(clisock);
            clisock.Close();
            continue;
         }
          std::cout << "client say:" << buf <<"\n";

          buf.clear();
          std::cin >> buf;
          clisock.Send(buf);
          if(clisock.Send(buf) == false)
          {
           s.Del(clisock);
           clisock.Close();
           continue;
          }
      }
    }
  }
  sock.Close(); 
  return -1;
}
