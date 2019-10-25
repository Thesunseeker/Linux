/*封装一个tcpsocket类向外提供简单接口,能够实现客户端服务端编程流程*/
/*1.创建套接字 2.绑定地址信息 3.开始监听/发起连接请求 4获取已完成链接 5发送数据 6接收数据 7关闭套接字*/
#include <iostream>
#include <string>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#define CHECK_RET(q) if((q) == false){return -1;}
class TcpSocket
{
  public:
    TcpSocket()
      :_sockfd(-1)
    {}
    ~TcpSocket(){
      Close();
    }
    bool Socket()
    {
      _sockfd = socket(AF_INET , SOCK_STREAM , IPPROTO_TCP);
      if(_sockfd < 0)
      {
        std::cerr << "scoket error\n";
        return false;
      }
      return true;
    }
    bool Bind( std::string &ip,uint16_t port)
    {
      struct sockaddr_in addr;
      addr.sin_family  = AF_FILE;
      addr.sin_port = htons(port);
      addr.sin_addr.s_addr = inet_addr(&ip[0]);
      socklen_t len = sizeof(struct sockaddr_in);
      int ret = bind(_sockfd , (struct sockaddr*)&addr , len);
      if(ret <0 )
      {
        std::cerr << "bind error\n";
        return false;
      }
      return true;
    }

    bool Listen(int backlog = 5)
    {
      int ret = listen(_sockfd , backlog);
      if(ret < 0 )
      {
        std::cerr << "listen error\n";
        return false;
      }
      return true;
    }

    bool Connect(const std::string &ip , uint16_t port)
    {
      int ret;
      struct sockaddr_in addr;
      addr.sin_family = AF_FILE;
      addr.sin_port = htons(port);
      addr.sin_addr.s_addr = inet_addr(&ip[0]);
      socklen_t len = sizeof(struct sockaddr_in);
      ret = connect(_sockfd , (struct sockaddr*)&addr , len);
      if(ret < 0)
      {
        std::cerr << "connect error\n";
        return false;
      }
      return true;
    }

    void SetFd(int fd)
    {
      _sockfd = fd;
    }

    int GetFd(int _sockfd)
    {
      return _sockfd;
    }

    bool Accept(TcpSocket &newsock)
    {
      struct sockaddr_in addr;
      socklen_t len = sizeof(struct sockaddr_in);
      //accept是一个阻塞函数；
      //当已完成连接队列中没有新连接的时候就会阻塞
      int fd = accept(_sockfd , (struct sockaddr*)&addr , &len);
      if(fd < 0)
      {
        std::cerr << "accept error\n";
        return false;
      }
      //接收一般发生在服务端,获取一个新的链接,若获取失败则应获取下一个不应退出.
      newsock.SetFd(fd);
      return true;
    }
    bool Send(std::string &buf)
    {
      int ret = send(_sockfd , &buf[0] , buf.size() , 0);
      //0为阻塞操作,当发送缓冲区满了后阻塞
      if(ret < 0)
      {
        std::cerr << "send error\n";
        return false;
      }
      return true;
    }
    bool Recv(std::string &buf)
    {
      //recv返回值为0不是为了表示返回值没有数据,而是为了表示链接断开(没有数据会阻塞)
      char tmp[4096] = {0};
      int ret = recv(_sockfd , tmp , 4096 , 0);
      if(ret < 0)
      {
        std::cerr << "recvfrom error\n";
        return false;
      }
      else if (ret == 0)
      {
        std::cerr << "peer shutdown\n";
        return false;
      }
      buf.assign(tmp, ret);
      return true;
    }

    bool Close()
    {
      if(_sockfd >= 0)
      {
        close(_sockfd);
        _sockfd = -1;
      }
      return true;
    }
  private:
    int _sockfd;
};
