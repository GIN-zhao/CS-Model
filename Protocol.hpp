#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <unordered_map>
#include <algorithm>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "Util.hpp"
#include "Log.hpp"

#define HTTP_VERSION "HTTP/1.0"
#define LINE_END "\r\n" // 行分隔符
#define SEP ": "
#define WEB_ROOT "wwwroot"     // web根目录，默认从该位置开始
#define HOME_PAGE "index.html" // 未指明具体文件资源时，默认返回当前路径下的index.html文件
#define PAGE_400 "400.html"
#define PAGE_403 "403.html"
#define PAGE_404 "404.html"
#define PAGE_500 "500.html"
#define PAGE_504 "504.html"

#define OK 200
#define MOVED_PERMANENTLY 301
#define TEMPORARY_REDIRECT 307
#define BAD_REQUEST 400
#define FORBIDDEN 403
#define NOT_FOUND 404
#define SERVER_ERROR 500
#define BAD_GATEWAY 504

static std::string Code2Desc(int code)
{
    std::string desc;
    switch (code)
    {
    case 200:
        desc = "OK";
        break;
    case 404:
        desc = "NOT FOUND";
        break;
    default:
        break;
    }

    return desc;
}

static std::string Suffix2Desc(const std::string &suffix)
{
    static std::unordered_map<std::string, std::string> suffix2desc = {
        {".html", "text/html"},
        {".css", "text/css"},
        {".xml", "text/xml"},
        {".js", "application/x-javascript"},
        {".jpg", "image/jpeg"}};

    if (suffix2desc.find(suffix) != suffix2desc.end())
    {
        // 找到了
        return suffix2desc[suffix];
    }

    return "text/html";
}
class HttpRequest
{
public:
    std::string _request_line;
    std::vector<std::string> _request_header;
    std::string _blank;
    std::string _request_body;
    // 解析后
    std::string _method;  // 请求方法
    std::string _uri;     // 资源标识
    std::string _version; // HTTP版本

    // 存储报头中的k: v
    std::unordered_map<std::string, std::string> _http_header_kv;
    int _content_length = 0; // 正文长度
    std::string _uri_path;
    std::string _uri_query; // ?后面的参数

    bool _cgi = false; // 是否需要使用cgi模式  GET方法带参数  POST方法
};
class HttpResponse
{
public:
    std::string _status_line;
    std::vector<std::string> _response_header;
    std::string _blank = LINE_END;
    std::string _response_body;

    // _status_line: version status_code status_desc 状态描述
    int _status_code = OK;
    int _fd = -1;                  // 最终要打开的文件
    int _body_size = 0;            // 文件的大小
    std::string _suffix = ".html"; // 文件后缀
};

// 读取请求，分析请求，构建响应
// IO通信
class EndPoint
{
public:
    EndPoint(int sock)
        : _sock(sock)
    {
    }
    void RecvHttpRequest()
    {
        if ((!RecvHttpRequestLine()) && (!RecvHttpRequestHeader()))
        {
            ParseHttpRequestLine();
            ParseHttpRequestHeader();
            // 读取正文
            RecvHttpRequestBody();
        }
    }
    void BuildHttpResponse()
    {
        auto &method = _http_request._method;
        auto &_path = _http_request._uri_path;
        auto &query = _http_request._uri_query;
        size_t pos = 0;
        if (method != "GET" && method != "POST")
        {
            // 非法请求
            LOG(WARNING, "method is not right");
            _http_response._status_code = BAD_REQUEST;
            goto END;
        }
        if (method == "GET")
        {
            // 是否带参 uri=path?
            if (_http_request._uri.find("?") != std::string::npos)
            {
                // 找到?，说明带有参数
                Util::CutString(_http_request._uri, _path, query, "?");
                // std::cout << _path << ":" << query << std::endl;
                //  webroot/test_cgi
                _http_request._cgi = true;
            }
            else
            {
                _path = _http_request._uri;
            }
        }
        else if (method == "POST")
        {
            // 使用cgi模式
            _http_request._cgi = true;
            _path = _http_request._uri;
        }
        // std::cout << __LINE__ << ": " + _path <<std::endl;
        //  path拼接前缀 webroot
        _path = WEB_ROOT + _path;
        // 未知名具体文件资源时，默认返回当前目录下的首页
        if (_path[_path.size() - 1] == '/')
        {
            _path += HOME_PAGE;
        }
        // std::cout << "debug: path:" + _path + " query:" + query << std::endl;
        struct stat st;
        if (stat(_path.c_str(), &st) == 0)
        {
            // 资源存在，判断是否是一个目录
            if (S_ISDIR(st.st_mode))
            {
                // 是一个目录，添加默认首页
                _path += "/";
                _path += HOME_PAGE;
                // 对st进行更新，获取文件大小
                stat(_path.c_str(), &st);
            }
            _http_response._body_size = st.st_size;
            // 判断请求是否是一个可执行程序
            // 拥有者具有可执行，所属组具有，其他人具有都能够证明是一个可执行程序
            if ((st.st_mode & S_IXUSR) && (st.st_mode & S_IXGRP) && (st.st_mode & S_IXOTH))
            {
                _http_request._cgi = true;
            }
        }
        else
        {
            // 资源不存在
            LOG(WARNING, _path + " Not Found");
            _http_response._status_code = NOT_FOUND;
            goto END;
        }
        // 提取后缀，默认为 ".html"
        pos = _path.rfind('.');
        if (pos != std::string::npos)
        {
            _http_response._suffix = _path.substr(pos);
        }

        if (_http_request._cgi)
        {
            // cgi模式 ProcessCgi   做数据处理
            _http_response._status_code = ProcessCgi();
        }
        else
        {
            // 非cgi模式，NoProcessCgi  不做数据处理，直接返回静态网页
            // 构建http响应，返回网页
            _http_response._status_code = ProcessNonCgi();
        }
    END:
        // 统一处理，构建响应
        BuildHttpResponseHelper();
        return;
    }
    void SendHttpResponse()
    {
        // LOG(INFO, "begin send!!!");
        //  发送状态行
        send(_sock, _http_response._status_line.c_str(), _http_response._status_line.size(), 0);
        // 发送报头
        for (auto &iter : _http_response._response_header)
        {
            send(_sock, iter.c_str(), iter.size(), 0);
        }
        // 发送空行
        send(_sock, _http_response._blank.c_str(), _http_response._blank.size(), 0);
        // 发送正文 使用sendfile，将一个文件的内容拷贝给另一个文件，不经过用户层，在内核区进行拷贝
        if (_http_request._cgi)
        {
            // LOG(INFO, "body");
            //  处理结果放在body中
            auto &body = _http_response._response_body;
            int size = 0;
            int total = 0;
            while ((size = send(_sock, body.c_str() + total, body.size() - total, 0)) > 0)
            {
                total += size;
            }
        }
        else
        {
            // LOG(INFO, "page");
            //  返回正常页面或者错误页面
            sendfile(_sock, _http_response._fd, nullptr, _http_response._body_size);
            // LOG(INFO, "close file");
            //  关闭文件
            close(_http_response._fd);
        }
    }
    bool IsStop()
    {
        // std::cout << "debug: " << _stop << std::endl;
        return _stop;
    }
    ~EndPoint()
    {
        if (_sock >= 0)
            close(_sock);
    }

private:
    bool RecvHttpRequestLine()
    {
        std::string &line = _http_request._request_line;
        // LOG(INFO, line);
        // std::cout << "debug: " << line.size() << " " << __LINE__ << std::endl;
        // std::cout << "debug: " << line[line.size()-2];
        // std::cout << "debug: " << line[line.size()-1];
        Util::ReadLine(_sock, line);
        // std::cout << "debug: " << line.size() << " " << __LINE__ << std::endl;
        if (line.size() > 0)
            line.pop_back(); // 去掉结尾的'\n'符号
        else
            _stop = true;
        // std::cout << "debug: " << _stop << std::endl;
        // LOG(INFO, line);
        return _stop;
    }
    bool RecvHttpRequestHeader()
    {
        while (1)
        {
            std::string line;
            if (Util::ReadLine(_sock, line) <= 0)
            {
                _stop = true;
                break;
            }
            if (line == "\n")
            {
                // 空行
                _http_request._blank = line;
                break;
            }
            // 去掉换行
            line.pop_back();
            _http_request._request_header.push_back(line);
            //  LOG(INFO, line);
        }

        return _stop;
    }
    void ParseHttpRequestLine()
    {
        // 解析 Get / HTTP/1.1
        std::stringstream ss(_http_request._request_line);
        ss >> _http_request._method >> _http_request._uri >> _http_request._version;
        auto &method = _http_request._method;
        // 将请求方法统一转化为大写
        std::transform(method.begin(), method.end(), method.begin(), toupper);
        // LOG(INFO, _http_request._method);
        // LOG(INFO, _http_request._uri);
        // LOG(INFO, _http_request._version);
    }
    void ParseHttpRequestHeader()
    {
        std::string key;
        std::string value;
        for (auto &s : _http_request._request_header)
        {
            // key: value
            if (Util::CutString(s, key, value, SEP))
                _http_request._http_header_kv.insert({key, value});
            // LOG(INFO, "debug:"+key);
            // LOG(INFO, "debug:"+value);
        }
    }

    // POST请求的处理
    bool IsNeedRecvHttpRequestBody()
    {
        auto &method = _http_request._method;
        if (method == "POST")
        {
            auto &header_kv = _http_request._http_header_kv;
            auto iter = header_kv.find("Content-Length");
            if (iter != header_kv.end())
            {
                _http_request._content_length = stoi(iter->second);
                //  std::cout << "debug: content-length " << _http_request._content_length << std::endl;
                return true;
            }
        }
        return false;
    }
    void RecvHttpRequestBody()
    {
        if (IsNeedRecvHttpRequestBody())
        {
            char ch;
            int content_length = _http_request._content_length;
            auto &http_request_body = _http_request._request_body;

            while (content_length)
            {
                ssize_t sz = recv(_sock, &ch, 1, 0);
                if (sz > 0)
                {
                    http_request_body += ch;
                    content_length--;
                }
                else if (sz == 0)
                {
                    // 客户端发送数据大小与Content-Length字段描述的不符
                    _stop = true;
                    break;
                }
                else
                {
                    // error
                    _http_response._status_code = SERVER_ERROR;
                    break;
                }
            }
        }
        LOG(INFO, _http_request._request_body);
    }
    int ProcessNonCgi()
    {
        // 打开文件
        _http_response._fd = open(_http_request._uri_path.c_str(), O_RDONLY);
        if (_http_response._fd >= 0)
        {
            return OK;
        }
        return NOT_FOUND;
    }
    int ProcessCgi()
    {
        // std::cout << "debug: use cgi model" << std::endl;
        auto &method = _http_request._method;
        auto &path = _http_request._uri_path;
        auto &query = _http_request._uri_query;   // GET url提交参数
        auto &body = _http_request._request_body; // POST 正文请求
        int content_length = _http_request._content_length;
        auto &code = _http_response._status_code;
        // 对于父进程 input是读入，output是写入
        int input[2];
        int output[2];
        // 创建两个管道
        if (pipe(input) < 0)
        {
            LOG(ERROR, "create output pipe error!");
            return SERVER_ERROR;
        }
        if (pipe(output) < 0)
        {
            LOG(ERROR, "create output pipe error!");
            return SERVER_ERROR;
        }
        pid_t pid = fork();
        if (pid == 0)
        {
            // 子进程
            close(input[0]);  // 关闭读，用来写
            close(output[1]); // 关闭写，用来读
            // 导入环境变量，便于程序替换后方便识别是何种请求方式
            std::string method_env = "METHOD=" + method;
            putenv((char *)method_env.c_str());
            if (method == "GET")
            {
                std::string query_env = "QUERY_STRING=" + query;
                putenv((char *)query_env.c_str());
            }
            else if (method == "POST")
            {
                // 将正文长度导入环境变量
                    
                std::string body_size_env = "CONTENT_LENGTH=" + std::to_string(content_length);
                putenv((char *)body_size_env.c_str());
            }
            // std::cout << "debug: content_length:" << atoi(getenv("CONTENT_LENGTH")) << std::endl;
            // std::cout << "method:" + method << std::endl;
            // std::cout << "path:" + path << std::endl;
            // std::cout << "debug: content_length:" << atoi(getenv("CONTENT_LENGTH")) << std::endl;
            //  对标准输入和标准输出进行重定向
            dup2(input[1], 1);
            dup2(output[0], 0);
            // 程序替换execl
            execl(path.c_str(), path.c_str(), nullptr);
            std::cerr << path << std::endl;
            // 失败就退出
            std::cerr << "execl error" << std::endl;
            exit(5);
        }
        else if (pid < 0)
        {
            // 创建失败
            LOG(ERROR, "fork error!");
            return SERVER_ERROR;
        }
        else
        {
            // 父进程
            close(input[1]);  // 关闭写，用来读
            close(output[0]); // 关闭读，用来写
            if (method == "POST")
            {
                int size = 0;
                int total = 0;
                // 将参数写进管道
                while ((size = write(output[1], body.c_str() + total, body.size() - total) > 0))
                {
                    total += size;
                }
            }
            // 读取管道
            char ch;
            while (read(input[0], &ch, 1) > 0)
            {
                _http_response._response_body += ch;
            }
            int status;
            pid_t ret = waitpid(pid, &status, 0);
            // LOG(INFO, "code="+std::to_string(code));
            if (ret > 0)
            {
                if (WIFEXITED(status))
                {
                    // 正常退出
                    if (WEXITSTATUS(status) == 0)
                    {
                        // 退出码正常
                        code = OK;
                    }
                    else
                    {
                        // 退出码不正常
                        // 5：程序替换失败
                        // 6: 请求方法不对
                        code = WEXITSTATUS(status) == 5 ? SERVER_ERROR : BAD_REQUEST;
                        LOG(INFO, "code=" + std::to_string(code));
                    }
                }
                else
                {
                    // 异常退出
                    code = SERVER_ERROR;
                    LOG(INFO, "code=" + std::to_string(code));
                }
            }
            else
            {
                // 等待失败
                code = SERVER_ERROR;
                LOG(INFO, "code=" + std::to_string(code));
            }
            // LOG(INFO, "code="+std::to_string(code));
            close(input[0]);
            close(output[1]);
        }
        return code;
    }
    void BuildOkResponseHeader()
    {
        std::string content_type_string = "Content-Type: ";
        content_type_string += Suffix2Desc(_http_response._suffix) + LINE_END;
        _http_response._response_header.push_back(content_type_string);
        std::string content_length_string = "Content-Length: ";
        if (_http_request._cgi)
        {
            // POST GET带参，根据响应正文的大小获取大小
            content_length_string += std::to_string(_http_response._response_body.size()) + LINE_END;
        }
        else
        {
            // 非cgi，获取页面， 根据body_size获取页面大小
            content_length_string += std::to_string(_http_response._body_size) + LINE_END;
        }
        _http_response._response_header.push_back(content_length_string);
    }
    void HandlerError(std::string page)
    {
        // LOG(INFO, "HandlerError");
        //  错误处理统一通过页面返回
        _http_request._cgi = false;
        // 打开文件
        // 错误页面统一放在wwwroot目录下
        page = WEB_ROOT + std::string("/") + page;
        LOG(INFO, "HandlerError:" + page);
        _http_response._fd = open(page.c_str(), O_RDONLY);
        if (_http_response._fd > 0)
        {
            // LOG(INFO, "HandlerError: open file");
            struct stat st;
            stat(page.c_str(), &st);
            _http_response._body_size = st.st_size;
            // std::cout << "size: " << st.st_size << "fd: " << _http_response._fd << std::endl;
            std::string content_type_string = "Content-Type: ";
            content_type_string += Suffix2Desc(".html") + LINE_END;
            _http_response._response_header.push_back(content_type_string);
            std::string content_length_string = "Content-Length: ";
            content_length_string += std::to_string(_http_response._body_size) + LINE_END;
            _http_response._response_header.push_back(content_length_string);
        }
    }
    void BuildHttpResponseHelper()
    {
        auto &code = _http_response._status_code;
        // 版本 状态码 状态描述
        _http_response._status_line += HTTP_VERSION;
        _http_response._status_line += " " + std::to_string(code);
        _http_response._status_line += " " + Code2Desc(code);
        _http_response._status_line += LINE_END; // 行分割符

        switch (code)
        {
        case OK:
            BuildOkResponseHeader();
            break;
        case BAD_REQUEST:
            HandlerError(PAGE_400);
            break;
        case FORBIDDEN:
            HandlerError(PAGE_403);
            break;
        case NOT_FOUND:
            HandlerError(PAGE_404);
            break;
        case SERVER_ERROR:
            HandlerError(PAGE_500);
            break;
        case BAD_GATEWAY:
            HandlerError(PAGE_504);
            break;
        default:
            break;
        }
    }

private:
    int _sock;
    bool _stop = false;
    HttpRequest _http_request;
    HttpResponse _http_response;
};

class CallBack
{
public:
    void operator()(int sock)
    {
        HandlerRequest(sock);
    }
    void HandlerRequest(int sock)
    {
        LOG(INFO, "Handler Request Begin");

        // std::string line;
        // Util::ReadLine(sock, line);
        // std::cout << line << std::endl;
        // std::cout << "get a new link... " << sock << std::endl;

#ifdef DEBUG
        char buf[1024];
        recv(sock, buf, sizeof(buf), 0);

        std::cout << "---------------------begin-------------------" << std::endl;
        std::cout << buf << std::endl;
        std::cout << "----------------------end--------------------" << std::endl;
#else
        EndPoint *ep = new EndPoint(sock);

        ep->RecvHttpRequest();
        if (!ep->IsStop())
        {
            LOG(INFO, "Recv No Error, Begin Build And Send");
            ep->BuildHttpResponse();
            ep->SendHttpResponse();
        }
        else
        {
            LOG(WARNING, "Recv Error, Stop Build And Send");
        }

        delete ep;
#endif
        LOG(INFO, "Handler Request End");
        close(sock);
    }
};
