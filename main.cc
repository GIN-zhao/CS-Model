#include <iostream>
#include <string>
#include <memory>
#include "HttpServer.hpp"
#include "Log.hpp"

static void Usage(std::string proc)
{
  std::cout << "Usage: \n\t" << proc << " port" << std::endl;
}

int main(int argc, char* argv[])
{
  if (argc != 2){
    Usage(argv[0]);
    exit(4);
  }
  
  //LOG(INFO, "run success");
  // 使用智能指针托管http_server资源
  std::shared_ptr<HttpServer> http_server(new HttpServer(atoi(argv[1])));
  
  http_server->InitServer();
  http_server->Loop();
  
  // TcpServer* svr = TcpS;erver::GetInstance(atoi(argv[1]));
  //std::cout << "hello world" << std::endl;
  //while (1);

  return 0;
}
