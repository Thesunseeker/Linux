#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#define WWW_ROOT "./www/"

class Boundary
{
  public:
    int64_t _start_addr;
    int64_t _data_len;
    std::string _name;
    std::string _filename;
};

bool GetHeader(const std::string &key , std::string &val)
{
  std::string body;
  char *ptr = getenv(key.c_str());
  if(ptr == NULL)
  {
    return false;
  }
  val = ptr;
  return true;
}

bool headerParse(std::string &header, Boundary &file)
{
  std::vector<std::string> list;
  boost::split(list , header , boost::is_any_of("\r\n"), boost::token_compress_on);
  for(unsigned long i = 0; i < list.size() ; i++)
  {
    std::string sep = ": ";
    size_t pos = list[i].find(sep);
    if(pos == std::string::npos)
    {
      return false;
    }

    std::string key = list[i].substr(0 , pos);
    std::string val = list[i].substr(pos + sep.size());
    if(key != "Content-Disposition")
    {
      continue;
    }

    std::string name_field = "fileupload";
    std::string filename_sep = "filename=\"";
    pos = val.find(name_field);
    if(pos == std::string::npos)
    {
      continue;
    }

    pos = val.find(filename_sep);
    if(pos == std::string::npos)
    {
      return false;
    }

    pos += filename_sep.size();
    size_t next_pos = val.find("\"", pos);
    if(next_pos == std::string::npos)
    {
      return false;
    }

    file._filename = val.substr(pos, next_pos - pos);
    file._name = "fileupload";
  }
  return true;
}

bool BoundaryParse(std::string &body, std::vector<Boundary> &list)
{
  std::string cont_b = "boundary=";
  std::string tmp;
  if(GetHeader("Content-Type", tmp) == false)
  {
    return false;
  }

  size_t pos = tmp.find(cont_b);
  if(pos == std::string::npos)
  {
    return false;
  }

  std::string boundary = tmp.substr(pos + cont_b.size());
  std::string dash = "--";
  std::string craf = "\r\n";
  std::string tail = "\r\n";
  std::string m_boundary = craf + dash + boundary;
  std::string f_boundary = dash + boundary + craf;

  size_t next_pos;
  pos = body.find(dash + boundary + craf);
  if(pos != 0)
  {
    std::cerr << "first boundary error\n";
    return false;
  }
  next_pos = pos + f_boundary.size();
  while(pos < body.size())
  {
    pos = body.find(tail, next_pos);
    if(pos == std::string::npos)
    {
      return false;
    }

    std::string header = body.substr(pos, next_pos - pos);
    next_pos = pos + tail.size();//数据起始地址
    pos = body.find(m_boundary, next_pos);//找\r\n--boundary
    if(pos == std::string::npos)
    {
      return false;
    }

    int64_t offset = next_pos;
    //下一个boundary的起始地址
    int64_t length = pos - next_pos;
    next_pos = pos + m_boundary.size();
    pos = body.find(craf , next_pos);
    if(pos == std::string::npos)
    {
      return false;
    }

    pos += craf.size();//pos指向这个下一个m_boundary的头部起始信息
    //若没有下一个m_boundary, 则pos指向数据结尾, pos= body.size() 循环退出
    Boundary file;
    file._data_len = length;
    file._start_addr = offset;
    //解析头部
    headerParse(header, file);
    list.push_back(file);
  }
  
  return true;
}

bool StorageFile(std::string &body , std::vector<Boundary> list)
{
  for(int i = 0 ; i < list.size() ; i++)
  {
    if(list[i]._name != "fileupload")
    {
      continue;
    }
    std::string realpath = WWW_ROOT + list[i]._filename;
    std::ofstream file(realpath);
    if(!file.is_open())
    {
      std::cerr << "open file " << realpath << "failed\n";
      return false;
    }
    file.write(&body[list[i]._start_addr] , list[i]._data_len);
    if(!file.good())
    {
      std::cerr << "write file error\n";
      return false;
    }
    file.close();
  }
  return true;
}

int main(int argc, char *argv[] , char *env[])
{
  
  return 0;
}
