#include "http.hpp"
#include "sereve.hpp"
int main()
{
  Server srv;
  srv.start(9000);
  return 0;
}
