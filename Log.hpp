#pragma once

#include <iostream>
#include <ctime>

// 日志级别 INFO WARNING ERROR FATAL  正常 警告 错误 致命错误
#define INFO    1
#define WARNING 2
#define ERROR   3
#define FATAL   4

// 替换，方便调用日志打印函数  # 数字转宏字符串
#define LOG(level, message) Log(#level, message, __FILE__, __LINE__)

void Log(std::string level, std::string message, std::string filename, int line)
{
  std::cout << "[" << level << "][" << message << "][" << time(nullptr) << "][" << filename << "][" << line << "]" << std::endl;
}
