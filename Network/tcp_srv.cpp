#include <stdlib.h>
#include <arpa/inet.h>
#include <signal.h>
#include "tcpsocket.hpp"
#include <sys/wait.h>

void sigcb(int signo)
{
  //SIGCHLD信号是一个非可靠信号, 多个进程同时退出可能会导致时间丢失, 导致有可能会有僵尸进程没有被处理
  //因此在一次事件回调中将能够处理的僵尸进程全部处理掉
  while(waitpid(-1 , NULL , WNOHANG) > 0);
}

int main(int argc , char* argv[])
{
  //创建套接字
  //绑定地址信息
  //开始监听
  //获取已完成链接
  //接收数据
  //发送数据
  //关闭套接字
  if(argc != 3)
  {
    std::cout << "./tcp_srv 192.168.115.129 9000\n";
    return -1;
  }
  signal(SIGCHLD , sigcb);
  std::string ip = argv[1];
  uint16_t port = atoi(argv[2]);
  TcpSocket lst_sock;

  CHECK_RET(lst_sock.Socket());
  CHECK_RET(lst_sock.Bind(ip , port));
  CHECK_RET(lst_sock.Listen());

  TcpSocket newsock;
  while(1)
  {
    bool ret = lst_sock.Accept(newsock);
    if (ret == false)
    {
      continue;
    }
    //创建子进程进行任务处理
    //任务分摊----每个进程只负责一个任务(主进程负责新连接获取,子进程负责与客户端进行同行)
    //稳定性高---子进程出问题不会影响到主进程服务器
    if(fork() == 0)
    {
      while(1)
      {
        std::string buf;
        ret = newsock.Recv(buf);
        if(ret == false)
        {
          newsock.Close();
          continue;
        }
        std::cout << "client say:" << buf << "\n";

        buf.clear();
        std::cin >> buf;
        newsock.Send(buf);
      }
      newsock.Close();
      exit(0);
    }
    newsock.Close();
  }

  lst_sock.Close();
  return 0;
}
