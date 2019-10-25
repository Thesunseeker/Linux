#include <sstream>
#include "tcpsocket.hpp"

int main(int argc, char *argv[])
{
  TcpSocket sock;
  CHECK_RET(sock.Socket());
  CHECK_RET(sock.Bind("0.0.0.0", 80));
  CHECK_RET(sock.Listen());
  while(1) {
    TcpSocket clisock;
    if (sock.Accept(clisock) == false) {
      continue;

    }
    std::string buf;
    clisock.Recv(buf);
    std::cout << "req:["<< buf << "]\n";

    std::string body = "<html><body><h1>超帅</h1></body></html>";
    body += "<meta http-equiv='content-type' content='text/html;charset=utf-8'>";
    std::string first = "HTTP/1.1 500 OK\r\n";
    std::stringstream ss;
    ss << "Content-Length: " << body.size() << "\r\n";
    ss << "Location: http://www.taobao.com/\r\n";
    std::string head = ss.str(); 
    std::string blank = "\r\n";

    clisock.Send(first);
    clisock.Send(head);
    clisock.Send(blank);
    clisock.Send(body);
    clisock.Close();

  }
  sock.Close();
  return 0;

}
