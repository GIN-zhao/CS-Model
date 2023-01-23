#pragma once

#include <iostream>
#include <queue>
#include <pthread.h>
#include <unistd.h>
#include "Task.hpp"
#include "Log.hpp"

#define NUM 5

class ThreadPool
{
private:
  ThreadPool(int max_pthread = NUM)
    :_stop(false)
     ,_max_thread(max_pthread)
  {}
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;
public:
  static ThreadPool* GetInstance(int num = NUM)
  {
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;// 静态的锁，不需要destroy 
    if (_tp == nullptr){
      pthread_mutex_lock(&lock);
      if (_tp == nullptr){
        _tp = new ThreadPool(num);
        if (!_tp->InitThreadPool())
          exit(-1);
      }
      pthread_mutex_unlock(&lock);
    }

    return _tp;
  }
  class CGarbo
  {
  public:
    ~CGarbo()
    {
      if (ThreadPool::_tp == nullptr){
        delete ThreadPool::_tp;
      }
    }
  };
  static void* Runtine(void* arg)
  {
    pthread_detach(pthread_self());
    ThreadPool* this_p = (ThreadPool*)arg;

    while (1){
      this_p->LockQueue();
      // 防止伪唤醒使用while
      while (this_p->IsEmpty()){
        this_p->ThreadWait();
      }
      Task* t;
      this_p->Get(t);
      this_p->UnlockQueue();
      // 解锁后处理任务
      t->ProcessOn();
      delete t;// 任务统一在堆上创建
    }
  }
  bool InitThreadPool()
  {
    pthread_mutex_init(&_mutex, nullptr);
    pthread_cond_init(&_cond, nullptr);
    pthread_t t[_max_thread];
    for(int i = 0; i < _max_thread; ++i)
    {
      if (pthread_create(t + i, nullptr, Runtine, this) != 0){
        LOG(FATAL, "ThreadPool Init Fail!");
        return false;
      }
    }
    LOG(INFO, "ThreadPool Init Success");
    return true;
  }
  void Put(Task* data)
  {
    LockQueue();
    _q.push(data);
    UnlockQueue();
    WakeUpThread();
  }
  void Get(Task*& data)
  {
    data = _q.front();
    _q.pop();
  }
  ~ThreadPool()
  {
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_cond);
  }
public:
  void LockQueue()
  {
    pthread_mutex_lock(&_mutex);
  }
  void UnlockQueue()
  {
    pthread_mutex_unlock(&_mutex);
  }
  void ThreadWait()
  {
    pthread_cond_wait(&_cond, &_mutex);
  }
  void WakeUpThread()
  {
    pthread_cond_signal(&_cond);
    //pthread_cond_broadcast(&_cond);
  }
  bool IsEmpty()
  {
    return _q.empty();
  } 
private:
  std::queue<Task*>  _q;
  bool _stop;
  int             _max_thread;
  pthread_mutex_t _mutex;
  pthread_cond_t  _cond;
  static ThreadPool* _tp;
  static CGarbo cg;// 内嵌垃圾回收
};

ThreadPool* ThreadPool::_tp = nullptr;
ThreadPool::CGarbo cg;
