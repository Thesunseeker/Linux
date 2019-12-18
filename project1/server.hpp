#ifndef __M_SERVER_H__
#define __M_SERVER_G__
#include "threadpool.hpp"
#include "tcpsocket.hpp"
#include "epollwait.hpp"
#include "htpptest.hpp"
#include <boost/filesystem.hpp>
#include <fstream>

#define WWW_ROOT "./www"


class Server
{
  private:
    TcpSocket  _lst_sock;
    Epoll _epoll;
    ThreadPool _pool;
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
            std::cerr << "new connect " << cli_sock.GetFd() << "\n";
          }
          else 
          {
            _epoll.Del(list[i]);
            ThreadTask tt(list[i].GetFd(), ThreadHandler);
            _pool.TaskPush(tt);
            std::cerr << "new request " << list[i].GetFd() << "\n";
          }
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
      std::cerr << "req parse status " << status << "\n";
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

    static int64_t str_to_digit(const std::string val)
    {
      std::stringstream tmp;
      tmp << val;
      int64_t dig = 0;
      tmp >> dig;
      return dig;
    }

    static bool HttpProcess(HttpRequest &req , HttpResponse &rsp)
    {
      //1.若请求是一个POST(上传文件)请求-多进程处理--则对进程CGI进行处理
      //2,若请求是一个GET(查看文件)请求--但是查询字符串不为空也是CGI处理
      //否则,若请求为GET,并且查询字符串为空,则为普通文件请求
      //若请求一个目录(查看列表) 
      //若请求文件(文件下载)
      std::string realpath = WWW_ROOT + req._path;
      //判断文件是否存在
      if(!boost::filesystem::exists(realpath))
      {
        rsp._status = 404;
        std::cerr << "realpath:[ " << realpath << " ]\n";
        return false;
      }
      std::cout << "realpath:[" << realpath << "]\n";
      std::cout << "method:[" << req._method << "]\n";
      if((req._method == "GET" && req._param.size() != 0) || req._method == "POST")
      {
        //对于当前则为一个文件上传请求
        CGIprocess(req, rsp);
      }
      else
      {
        //否则是一个基本的文件下载/目录列表请求
        if(boost::filesystem::is_directory(realpath))
        {
          //判断是一个目录列表请求, 查看目录列表
          ListShow(req, rsp);
          rsp.SetHeader("Content-Type" , "text/html");
        }
        else
        {
          //普通文件下载请求
          RangeDownload(req, rsp);
          return true;
        }
      }
      rsp._status = 200;
      return true;
    }

    static bool RangeDownload(HttpRequest &req, HttpResponse &rsp)
    {
      std::string realpath = WWW_ROOT + req._path;
      int64_t data_len = boost::filesystem::file_size(realpath);
      int64_t last_mtime = boost::filesystem::last_write_time(realpath);
      std::string etag = std::to_string(data_len) + std::to_string(last_mtime);
      auto it = req._headers.find("Range");
      if(it == req._headers.end())
      {
        Download(realpath ,0 , data_len, rsp._body);
        rsp._status = 200;
      }
      else 
      {
        std::string range = it->second;
        std::string unit = "bytes";
        size_t pos = range.find(unit);
        if(pos == std::string::npos)
        {
          return false;
        }
        pos += unit.size();
        size_t pos2 = range.find("-", pos);
        if(pos2 == std::string::npos)
        {
          return false;
        }
        std::string start = range.substr(pos, pos2 - pos);
        std::string end = range.substr(pos2 + 1);
        int64_t dig_start, dig_end;
        dig_start = str_to_digit(start);
        if(end.size() == 0)
        {
          dig_end = data_len - 1;
        }
        else 
        {
          dig_end = str_to_digit(end);
        }
        int64_t range_len = dig_end - dig_start + 1;
        Download(realpath , dig_start , range_len , rsp._body);
        std::stringstream tmp;
        tmp << "bytes" << dig_start << "-" << dig_end << "/" << data_len;
        rsp.SetHeader("Content-Range" ,tmp.str());
        rsp._status = 206;
      }
      rsp.SetHeader("Content-Type", "application/octet-stream");
      rsp.SetHeader("Accept-Ranges", "bytes");
      rsp.SetHeader("ETag", etag);
      return true;
    }

    static bool Download(std::string &path, int64_t start , int64_t len , std::string &body)
    {
      body.resize(len);
      std::ifstream file(path);
      if(!file.is_open())
      {
        std::cerr << "open file error\n";
        return false;
      }
      file.seekg(start, std::ios::beg);
      file.read(&body[0], len);
      if(!file.good())
      {
        std::cerr << "open file error\n";
        return false;
      }
      file.close();
      return true;
    }

    static bool CGIprocess(HttpRequest &req, HttpResponse &rsp)
    {
      //通过管道传送信息
      //将头部和正文分开传递, 建立两个管道进行双工通信, 一个管道传数据,一个管道拿数据
      //通过环境变量传递头部信息 ,管道传送正文信息
      int pipe_in[2] , pipe_out[2];
      if(pipe(pipe_in) < 0|| pipe(pipe_out) < 0)
      {
        std::cerr << "creat pipe error\n";
        return false;
      }
      //创建子进程, 并在子进程里设置环境变量
      int pid = fork();
      if(pid < 0)
      {
        return false;
      }
      else if (pid == 0 )
      {
        //将管道不用的一端关闭掉
        //用于父进程读, 子进程写, 将读端关闭
        close(pipe_in[0]);
        //用于父进程写, 子进程读 ,将写端关闭
        close(pipe_out[1]);
        //重定向将子进程的标准输出重定向到标准输入管道的写端
        dup2(pipe_in[1] , 1);
        //重定向将子进程的标准输入重定向到标准输入管道的读端
        dup2(pipe_out[0] , 0);
        //传递环境变量
        setenv("METHOD", req._method.c_str() , 1);
        setenv("PATH" , req._path.c_str(), 1);
        for (auto i : req._headers)
        {
          setenv(i.first.c_str(), i.second.c_str(), 1);
        }
        std::string realpath = WWW_ROOT + req._path;
        //程序替换
        execl(realpath.c_str(), realpath.c_str(), NULL);
        exit(0);
      }
      close(pipe_in[1]);
      close(pipe_out[0]);
      //父进程
      write(pipe_out[1] , &req._body[0] , req._body.size());

      //从管道读取数据
      while(1)
      {
        char buf[1024] = {0};
        int ret = read(pipe_in[0], buf , 1024);
        if(ret == 0)
        {
          break;
        }
        buf[ret] = '\0';
        rsp._body += buf;
      }
      close(pipe_in[0]);
      close(pipe_out[1]);
      return true;
    } 

    static bool ListShow(HttpRequest &req, HttpResponse &rsp)
    {
      //例如./www/testdir/a.txt - >去掉./www-> /testdir/a.txt
      std::string realpath = WWW_ROOT + req._path;
      std::string req_path = req._path;
      std::stringstream tmp;
      tmp << "<html><head><style>";
      tmp << "*{margin : 0;}";
      tmp << ".main-window {height : 100%; width : 80%; margin : 0 auto;}";
      tmp << ".upload{position : relative;height : 20%;width : 100%;background-color : #33c0b9; text-align:center;}";
      tmp << ".listshow{position : relative;height : 80%;width : 100%; background-color : #6fcad6;}";
      tmp << "</style></head>";
      tmp << "<body><div class='main-window'>";
      tmp << "<div class='upload'>";

      tmp << "<form action='/upload' method='POST'";
      tmp << "enctype='multipart/form-data'>";
      tmp << "<div class='upload-btn'>";
      tmp << "<input type='file' name='fileupload'>";
      tmp << "<input type='submit' name='submit' >";
      tmp << "</div></form>";

      tmp << "</div><hr />";
      tmp << "<div class='listshow'><ol>";
      //........组织每个文件信息节点

      //定义一个目录迭代器
      boost::filesystem::directory_iterator begin(realpath);
      boost::filesystem::directory_iterator end;
      for(; begin != end; ++begin)
      {
        int64_t ssize, mtime;
        std::string pathname = begin->path().string();
        std::string name = begin->path().filename().string();
        std::string uri = req_path + name;

        //若果是一个目录,进入目录的处理方式
        if(boost::filesystem::is_directory(pathname))
        {
          mtime = boost::filesystem::last_write_time(begin->path());//获取文件最后一次修改时间
          tmp << "<li><strong><a href='";
          tmp <<  uri << "'>";
          tmp << name << "/";
          tmp << "</a><br /></strong>/";
          tmp << "<small>flietype: director";
          tmp << "</small></li>";
        }
        else
        {
          mtime = boost::filesystem::last_write_time(begin->path());//获取文件最后一次修改时间
          ssize = boost::filesystem::file_size(begin->path());

          tmp << "<li><strong><a href='";
          tmp << uri << "'>";
          tmp << name;
          tmp << "</a><br /></strong>";
          tmp << "<small>modified: ";
          tmp << mtime;
          tmp << "<br /> flietype: application-octream ";
          tmp << ssize / 1024 << "kbytes";
          tmp << "</small></li>";
        }
      }
      tmp<< "</ol></div><hr /></div></body></html>";
      rsp._body = tmp.str();
      rsp._status = 200;
      rsp.SetHeader("Content-Type", "text/html");
      return true;
    }
};

#endif
