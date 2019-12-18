#ifndef __M_HTTP_H__
#define __M_HTTP_H__
#include <unordered_map>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include "tcpsocket.hpp"

class HttpRequest
{
  public:
    std::string _method;
    std::string _path;
    std::unordered_map<std::string ,std::string> _param;
    std::unordered_map<std::string ,std::string> _headers;
    std::string _body;
  private:
    bool RecvHeader(TcpSocket &sock,std::string &header)
    {
      //1.探测性接收大量数据 , 判断是否由
      while(1)
      {
        std::string tmp;
        if(sock.RecPeek(tmp) == false)
        {
          return false;
        }
        //std::cout<< "tmp:[" << tmp << "]\n";
        //2.判断是否包含整个头部/r/n/r/n
        size_t pos;
        pos = tmp.find("\r\n\r\n", 0);
//        std::cout << "tmp:["<<tmp<<"]\n";
        std::cout << "pos:["<<pos<<"]\n";
        //3.判断当前数据长度,
        if(pos == std::string::npos && tmp.size() == 8192)
        {
          return false;
        }
        else if(pos != std::string::npos)
        {
          //4.若包含整个头部则,直接获取头部;
          header.assign(&tmp[0], pos);
          size_t header_length = pos + 4 ;
          sock.Recv(tmp ,header_length);
          //std::cout << "header:["<< header<<"]\n";
          return true;
        }
      }
    } 

    bool FirstLineParse(std::string &line)
    {
      //GET, HTTP /1.1
      //空格进行分割
      //首行的list
      std::vector<std::string> line_list;
      boost::split(line_list ,line, boost::is_any_of(" "), boost::token_compress_on);
      //首行信息非3个,则报错
      if(line_list.size() != 3)
      {
        std::cerr << "parse first line error\n";
        return false;
      }
     _method = line_list[0];

     //url中找路径
     size_t pos = line_list[1].find("?" , 0);
     if(pos == std::string::npos)
     {
       _path = line_list[1];
     }
     else{
       _path = line_list[1].substr(0, pos);
       std::string query_string = line_list[1].substr(pos+1);
       //query_string :key=val&key=val
       std::vector<std::string> param_list;
       boost::split(param_list, query_string,boost::is_any_of("&"),boost::token_compress_on);
       for(auto i : param_list)
       {
         size_t param_pos = -1;
         param_pos = i.find("=");
         if(param_pos == std::string::npos)
         {
           std::cerr << "parse param error\n";
           return false;
         }
         std::string key = i.substr(0,param_pos);
         std::string val = i.substr(pos+1);
         _param[key] = val;
       }
     }
      return true;
    }

    bool PathIsLegal();

  public:

    int RequestParse(TcpSocket &sock)
    {
      //1.获取接收http头部 
      std::string header;
      if(RecvHeader(sock,header) == false)
      {
        return 400;
      }
      //2.对整个头部按照\r\n进行分割--得到一个list
      std::vector<std::string> header_list;
      boost::split(header_list , header, boost::is_any_of("\r\n"), boost::token_compress_on);
      for(unsigned long i = 0; i < header_list.size(); i++)
      {
        std::cout << "list[i] = [" << header_list[i] << "]\n";
      }
      //3.list[0] =----首行,进行首行解析
      if(FirstLineParse(header_list[0]) == false)
      {
        return 400;
      }

      //4.头部分割解析
      //key:val\r\n
      size_t pos = 0;
      for(unsigned long i = 1; i < header_list.size(); i++)
      {
        pos = header_list[i].find(": ");
        if(pos == std::string::npos)
        {
          std::cerr << "header parse error\n";
          return false;
        }
        std::string key = header_list[i].substr(0,pos);
        std::string val = header_list[i].substr(pos+2);
        _headers[key] = val;
      }

      //5.请求校验信息
    /*  
        std::cout << "_method:["<< _method <<"]\n";
        std::cout << "path:["<< _path << "]\n";
        for(auto i : _param)
        {
        std::cout << i.first<< " = " << i.second << "\n";
        }
        for(auto i : _headers)
        {
        std::cout << i.first<< " = " << i.second << "\n";
        }
    */
      //6.接收正文信息
      auto it =_headers.find("Content-Length");
      if(it != _headers.end())
      {
        std::stringstream tmp;
        tmp << it->second;
        //stringstread---将一个string对象转换成一个int64_t 类型的对象
        int64_t file_len;
        tmp >> file_len;
        sock.Recv(_body, file_len);
      }
      return 200;
    }
};

class HttpResponse
{
  public:
    int _status;
    std::unordered_map<std::string , std::string> _headers;
    std::string _body;
  private:
    std::string GetDesc()
    {
      switch(_status)
      {
        case 400: return "bad request";
        case 404: return "not found";
        case 200: return "OK";
      }
      return "Unknow";
    }

  public:
    bool SetHeader(const std::string &key , const std::string &val)
    {
      _headers[key] = val;
      return true;
    }

    bool ErrorProcess(TcpSocket &sock)
    {
      return true;
    }

    bool NormalProcess(TcpSocket &sock)
    {
      std::stringstream tmp;

      tmp << "HTTP/1.1" << " " << _status << " "<< GetDesc();
      tmp << "\r\n";
      if(_headers.find("Content-Length") == _headers.end())
      {
        //没有找到则自己添加一个Content-Length
        std::string len = std::to_string(_body.size());
        _headers["Content-Length"] =len;
      }
      for(auto i : _headers)
      {
        tmp << i.first<< ": " << i.second << "\r\n";
      }
      //空行
      tmp << "\r\n";
      sock.Send(tmp.str());
      sock.Send(_body);
      return true;
    }
};

#endif
