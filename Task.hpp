#pragma once

#include "Protocol.hpp"

class Task
{
public:
  Task(int sock)
    :_sock(sock)
  {}
  // 处理任务
  void ProcessOn()
  {
    _handlerRequest(_sock);
  }
private:
  int _sock;
  CallBack _handlerRequest;// 设置回调，处理请求与构建响应
};
