#include <iostream>
#include <stdlib.h>
#include <string>
#include <unistd.h>


bool GetQueryString(std::string& query)
{
  std::string method = getenv("METHOD");
  std::cerr << "method: " + method << std::endl;
  if (method == "GET"){
    query = getenv("QUERY_STRING");
  //  cerr << "query_string: " + query << endl;
    return true;
  }
  else if (method == "POST"){
    int content_length = atoi(getenv("CONTENT_LENGTH"));
  //  std::cerr << "debug: content_length:" << content_length << std::endl;
    char ch;
    while (content_length){
      read(0, &ch, 1);
      query += ch;
      content_length--;
    }
    std::cerr << "query_string: " + query << std::endl;
    return true;
  }
  else{
    return false;
  }
}

void CutString(const std::string& in, const std::string sep, std::string& out1, std::string& out2)
{
  size_t pos = in.find(sep);
  if (pos != std::string::npos){
    out1 = in.substr(0, pos);
    out2 = in.substr(pos+sep.size());
  }
}
int main()
{
  std::string query;
  if (!GetQueryString(query)){
    exit(7);// 请求方法出错
  }
  std::string s1;
  std::string s2;
  CutString(query, "&", s1, s2);
  
  std::string name1;
  std::string value1;
  CutString(s1, "=", name1, value1);

  std::string name2;
  std::string value2;
  CutString(s2, "=", name2, value2);
  if (value1.size() == 0 || value2.size() == 0){
    exit(7);
  }
  // 写入管道
  //std::cout << name1 + ":" + value1 << std::endl;
  //std::cout << name2 + ":" + value2 << std::endl;
  int num1 = stoi(value1), num2 = stoi(value2);
  if (num2 == 0){
    std::cout << "<head><meta charset=\"UTF-8\"></head>" << std::endl;
    std::cout << "<body><h1>除零错误</h1></body>" << std::endl;
  }
  else{
    std::cout << "<head><meta charset=\"UTF-8\"></head>" << std::endl;
    std::cout << "<body><h1>计数结果如下：</h1><br/>" << std::endl;
    std::cout << "<h2>" << num1 << "+" << num2 << "=" << (double)num1+num2 <<  "</h2><br/>" << std::endl;
    std::cout << "<h2>" << num1 << "-" << num2 << "=" << (double)num1-num2 <<  "</h2><br/>" << std::endl;
    std::cout << "<h2>" << num1 << "*" << num2 << "=" << (double)num1*num2 <<  "</h2><br/>" << std::endl;
    std::cout << "<h2>" << num1 << "/" << num2 << "=" << (double)num1/num2 <<  "</h2><br/><body/>" << std::endl;
  }
  
  std::cerr << name1 + ":" + value1 << std::endl;
  std::cerr << name2 + ":" + value2 << std::endl;
  
  return 0;
}
