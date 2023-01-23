#pragma once

#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <string>

class Util
{
public:
    static int ReadLine(int sock, std::string &out)
    {
        char ch = '*';
        while (ch != '\n')
        {
            ssize_t sz = recv(sock, &ch, 1, 0);

            // std::cout << "debug: " << sz << " " << ch << " " << __LINE__ << std::endl;
            if (sz > 0)
            {
                // 三种情况都转为\n
                // 1. \r\n
                // 2. \n
                // 3. \r
                if (ch == '\r')
                {
                    // 使用MSG选项进行窥探，不取走接受缓冲区的数据
                    recv(sock, &ch, 1, MSG_PEEK);
                    if (ch == '\n')
                    {
                        // 情况1
                        // 窥探成功，将该数据从接受缓冲区取走
                        recv(sock, &ch, 1, 0);
                    }
                    else
                    {
                        // 情况2
                        ch = '\n';
                    }
                }
                // 正常或者转换后
                out += ch;
            }
            else if (sz == 0)
            {
                break; // return 0;
            }
            else
            {
                return -1;
            }
        }
        std::cout << "Request Line :=====" << out << endl;

        return out.size();
    }
    static bool CutString(const std::string &s, std::string &sub1_out, std::string &sub2_out, std::string sep)
    {
        size_t pos = s.find(sep);
        if (pos != std::string::npos)
        {
            sub1_out = s.substr(0, pos);
            sub2_out = s.substr(pos + sep.size());
            return true;
        }

        return false;
    }
};
