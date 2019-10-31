#include "threadpool.hpp"
#include "tcpsocket.hpp"
#include "epollwait.hpp"
#include "htpptest.hpp"
#include <boost/filesystem.hpp>

#define WWW_ROOT "./www"


class Server
{
  public:
    bool Start(int port)
    {
      bool ret;
      ret = _lst_sock.SockInit(port);
      if(ret == false)
      {
        return false;
      }
      ret = _pool.PoolInit();
      if(ret == false)
      {
        return false;
      }
      ret = _epoll.Init();
      if(ret == false)
      {
        return false;
      }
      _epoll.Add(_lst_sock);
      while(1)
      {
        std::vector<TcpSocket> list;
        ret = _epoll.Wait(list);
        if(ret == false)
        {
          continue;
        }
        for(unsigned long i = 0 ;i < list.size() ; i++)
        {
          if(list[i].GetFd() == _lst_sock.GetFd())
          {
            TcpSocket cli_sock;
            ret = _lst_sock.Accept(cli_sock);
            if(ret == false)
            {
              continue;
            }
            cli_sock.SetNonBlock();
            _epoll.Add(cli_sock);
          }
            ThreadTask tt(list[i].GetFd(), ThreadHandler);
            _pool.TaskPush(tt);
            _epoll.Del(list[i]);
        }
      }
      _lst_sock.Close();
      return true;
    }
  public:
    static void ThreadHandler(int sockfd)
    {
      TcpSocket sock;
      sock.SetFd(sockfd);
      HttpRequest req;
      HttpResponse rsp;
      int status = req.RequestParse(sock);
      //接受一个请求并进行解析
      if( status != 200 )
      {
        //解析失败则直接响应错误
        rsp._status = status;
        rsp.ErrorProcess(sock);
        sock.Close();
        return;
      }
      std::cout << "-----------------\n";
      //根据请求进行处理, 将处理结果填充到rsp中
      HttpProcess(req,rsp);
      //将处理响应结果返回给客户端
      rsp.NormalProcess(sock);
      //当前采用短连接,直接处理完毕后关闭套接字
      sock.Close();
      return;
    }
    static bool HttpProcess(HttpRequest &req , HttpResponse &rsp)
    {
      //1.若请求是一个POST请求----则对进程CGI进行处理
      //2,若请求是一个GET请求-----但是查询字符串不为空也是CGI处理
      //否则,若请求为GET,并且查询字符串为空,则为普通文件请求
      //若请求一个目录(查看列表) 
      //若请求文件(文件下载)
      std::string realpath = WWW_ROOT + req._path;
      if(!boost::filesystem::exists(realpath))
      {
        rsp._status = 404;
        std::cerr << "realpath:[ " << realpath << " ]\n";
        return false;
      }
      if((req._method == "GET" && req._param.size() != 0) || req._method == "POST")
      {
        //对于当前则为一个文件上传请求
        for(auto i :req._headers)
        {
          std::cout << i.first << "=" << i.second <<"\n";
        }
        std::cout << "body:["<< req._body << "]\n";
      }
      else{
        //否则是一个基本的文件下载/目录列表区请求
        if(boost::filesystem::is_directory(realpath))
        {
          //查看目录列表请求
          ListShow(realpath, rsp._body);
          rsp.SetHeader("Content-Type" , "text/html");
        }
      {
          //普通文件下载请求
        }
      }
      rsp._status = 200;
      return true;
    }
    static bool ListShow(std::string &path, std::string &body)
    {
      //例如./www/testdir/a.txt - >去掉./www-> /testdir/a.txt
      std::string www = WWW_ROOT;
      size_t pos = path.find(www);
      if(pos == std::string::npos)
      {
        return false;
      }
      std::string req_path = path.substr(www.size()); 
      std::stringstream tmp;
      tmp << "<html><head><style>";
      tmp << "*{margin : 0;}";
      tmp << ".main-window {height : 100%; width : 80%; margin : 0 auto;}";
      tmp << ".upload{position : relative;height : 20%;width : 100%;background-color : #33c0b9; text-align:center;}";
      tmp << ".listshow{position : relative;height : 80%;width : 80%;width : 80%;background-color : #6fcad6;}";
      tmp << "</style></head><body>";
      tmp << "<div class='main-window'>";
      tmp << "<div class='upload'>";

      tmp << "<form action='/upload' method='POST' enctype='multipart/form-data'>";
      tmp << "<div class='upload-bth'>";
      tmp << "<input type='file' name='fileupload'>";
      tmp << "<input type='submit' name='submit' value='上传'>";
      tmp << "</div></form>";

      tmp << "</div><hr />";
      tmp << "<div class='listshow'><ol>";
      //........组织每个文件信息节点
      boost::filesystem::directory_iterator begin(path);
      boost::filesystem::directory_iterator end;
      for(; begin != end; ++begin)
      {
        std::string pathname = begin->path().string();
        std::string name = begin->path().filename().string();
        std::string uri = req_path + name;
        int64_t mtime = boost::filesystem::last_write_time(begin->path());//获取文件最后一次修改时间
        int64_t ssize = boost::filesystem::file_size(begin->path());

        if(boost::filesystem::is_directory(pathname))
        {
          tmp << "<li><strong><a href='";
          tmp <<  uri << "'>";
          tmp << name << "/";
          tmp << "</a></strong><br />";
          tmp << "<small> ";
          tmp << "<small> flietype: director";
          tmp << "</small></li>";
        }else{
          std::cout << "pathname:[" <<pathname<< "]\n";
          std::cout << "mtime:[" <<mtime<< "]\n";
          std::cout << "ssize:[" <<ssize<< "]\n";

          tmp << "<li><strong><a href='";
          tmp << uri << "'>";
          tmp << name;
          tmp << "</a></strong><br />";
          tmp << "<small> modified: ";
          tmp << mtime;
          tmp << "<br /><small> flietype: application-octream ";
          tmp << ssize / 1024 << "kbytes";
          tmp << "</small></li>";
        }
        //........组织每个文件信息节点
        tmp<< "</ol></div><hr /></div></body></html>";
        body = tmp.str();
        return true;

      }
      return true;
    }
  private:
    TcpSocket  _lst_sock;
    Epoll _epoll;
    ThreadPool _pool;
};


