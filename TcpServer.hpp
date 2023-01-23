#pragma once

#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <pthread.h>
#include "Log.hpp"

#define BACKLOG 5

class TcpServer
{
public:
  void InitServer()
  {
    Socket();
    Bind();
    Listen();
    LOG(INFO, "TcpServer Init Success");
  }
  static TcpServer* GetInstance(int port)
  {
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;// 静态的锁，不需要destroy 
    if (_svr == nullptr){
      pthread_mutex_lock(&lock);
      if (_svr == nullptr){
        _svr = new TcpServer(port);
        _svr->InitServer();
      }
      pthread_mutex_unlock(&lock);
    }

    return _svr;
  }
  class CGarbo
  {
  public:
    ~CGarbo()
    {
      if (TcpServer::_svr == nullptr){
        delete TcpServer::_svr;
      }
    }
  };
  int GetListenSock()
  {
    return _listen_sock;
  }
  ~TcpServer()
  {
    if (_listen_sock >= 0) close(_listen_sock);
  }
private:
  // 构造私有
  TcpServer(int port)
    :_port(port)
     ,_listen_sock(-1)
  {}
  // 禁止拷贝
  TcpServer(const TcpServer&) = delete;
  TcpServer& operator=(const TcpServer&) = delete;
  void Socket()
  {
    _listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (_listen_sock < 0){
      LOG(FATAL, "create socket error!");
      exit(1);
    }
    LOG(INFO, "create socket success");
    // 将套接字设置为可以地址复用
    int opt = 1;
    setsockopt(_listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  }
  void Bind()
  {
    struct sockaddr_in local;
    memset(&local, 0, sizeof(local));

    local.sin_family = AF_INET;
    local.sin_port = htons(_port);
    local.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(_listen_sock, (struct sockaddr*)&local, sizeof(local)) < 0){
      LOG(FATAL, "bind error!");
      exit(2);
    }
    LOG(INFO, "bind success");

  }
  void Listen()
  {
    if (listen(_listen_sock, BACKLOG) < 0){
      LOG(FATAL, "listen error!");
      exit(3);
    }
    LOG(INFO, "lieten success");
  }
private:
  int _port;
  int _listen_sock;
  static TcpServer* _svr;// 单例
  static CGarbo _cg;// 内嵌垃圾回收
};

TcpServer* TcpServer::_svr = nullptr;
TcpServer::CGarbo _cg;
