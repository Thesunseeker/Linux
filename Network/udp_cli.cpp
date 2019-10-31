/*封装实现一个udpsocket类 ; 向外提供更加容易使用的udp接口来实现udp通信流程*/
#include <unistd.h>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#define CHECK_RET(q) if((q) ==false ){return false;}
class UdpSocket
{
  public:
    UdpSocket()
      :_sockfd(-1)
    {}
    ~UdpSocket()
    {
      Close();
    }
    bool Socket()
    {
      _sockfd = socket(AF_INET , SOCK_DGRAM , IPPROTO_UDP);
      if(_sockfd < 0)
      {
        std::cerr << "socket error\n";
        return false;
      }
      return true;
    }
    bool Bind(const std::string &ip , uint16_t port)
    {
      struct sockaddr_in addr;
      addr.sin_family = AF_INET;
      addr.sin_port = htons(port);
      addr.sin_addr.s_addr = inet_addr(ip.c_str());
      socklen_t len = sizeof(struct sockaddr_in);
      int ret = bind(_sockfd , (struct sockaddr*)&addr , len);
      if(ret < 0)
      {
        std::cerr << "bind error\n";
        return false;
      }
      return true;
    }//给一个字符Ip
    bool Recv(std::string &buf , std::string &ip , uint16_t &port)
    {
      char tmp[4096];
      struct sockaddr_in peeraddr;//对端地址
      socklen_t len = sizeof(peeraddr);
      int ret = recvfrom(_sockfd , tmp , 4096 , 0 , (struct sockaddr*)&peeraddr, &len);
      if(ret < 0)
      {
        std::cerr << "recvfrom error\n";
        return false;
      }
      buf.assign(tmp , ret);//开辟空间从tmp拷贝ret长的数据到buf中
      port = ntohs(peeraddr.sin_port);
      ip = inet_ntoa(peeraddr.sin_addr);//inet_ntoa返回一个字符串的首地址 , 它是一个非线程安全的接口 
      //它将网络字节序的整数IP地址转换为字符串IP地址
      return true;
    }
    bool Send(std::string &data , std::string &ip , uint16_t port)
    {
      struct sockaddr_in addr;
      addr.sin_family = AF_INET;
      addr.sin_port = htons(port);
      addr.sin_addr.s_addr = inet_addr(ip.c_str());
      socklen_t len = sizeof(struct sockaddr_in);
      int ret = sendto(_sockfd , &data[0] , data.size() , 0 , (struct sockaddr*)&addr , len);
      if(ret < 0 )
      {
        std::cerr << "sendto error\n";
        return -1;
      }
      return true;
    }
    bool Close()
    {
      if(_sockfd >= 0 )
      {
        close(_sockfd);
        _sockfd = -1;
      }
      return true;

    }
  private:
    int _sockfd;
};

int main(int argc , char *argv[])
{
  if(argc != 3)
  {
    std::cout << "./udp_cli serverip serverport\n";
    return -1;
  }
  std::string srv_ip = argv[1];
  uint16_t srv_port = atoi(argv[2]);
  UdpSocket sock;
  
  CHECK_RET(sock.Socket());
//  CHECK_RET(sock.Bind("192.168.115.129" , 8000));
  while(1)
  {
    std::string buf;
    std::cin >> buf;
    CHECK_RET(sock.Send(buf ,srv_ip, srv_port));
    buf.clear();
    CHECK_RET(sock.Recv(buf ,srv_ip, srv_port));
    std::cout << "server sat:" << buf << "\n";
  }
  CHECK_RET(sock.Close());
  return 0;
}
