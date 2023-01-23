#pragma once

#include <signal.h>
#include "TcpServer.hpp"
#include "Log.hpp"
#include "ThreadPool.hpp"
#include "Task.hpp"

#define PORT 8081


class HttpServer
{
public:
  HttpServer(int port = PORT, int num = NUM)
    :_port(port)
     ,_num(num)
  {}
  void InitServer()
  {
    signal(SIGPIPE, SIG_IGN);//忽略该信号，否则服务器写入时，对端关闭会发送该信号导致服务器关闭
  }
  void Loop()
  {
    LOG(INFO, "Loop Begin");
    // 两个组件
    TcpServer* tsvr = TcpServer::GetInstance(_port);
    ThreadPool* tp  = ThreadPool::GetInstance(_num);
    int lsock = tsvr->GetListenSock();

    struct sockadd_in* peer;
    socklen_t len = sizeof(peer);

    while (!_stop){
      int sock = accept(lsock, (struct sockaddr*)&peer, &len);
      if (sock < 0){
        continue;
      }
      LOG(INFO, "Get a new link");
      // 构建任务
      Task* t = new Task(sock);
      tp->Put(t);
    }
    LOG(INFO, "Loop End");
  }
private:
  int _port;
  int _num;
  static bool _stop;
};

bool HttpServer::_stop = false;
