#include <iostream>
#include <cstring>
#include <string>
#include <sys/sendfile.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <mysql.h>

std::string UrlEncode(const std::string &szToEncode)
{
    std::string src = szToEncode;
    char hex[] = "0123456789ABCDEF";
    std::string dst;

    for (size_t i = 0; i < src.size(); ++i)
    {
        unsigned char cc = src[i];
        if (isascii(cc))
        {
            if (cc == ' ')
            {
                dst += "%20";
            }
            else
                dst += cc;
        }
        else
        {
            unsigned char c = static_cast<unsigned char>(src[i]);
            dst += '%';
            dst += hex[c / 16];
            dst += hex[c % 16];
        }
    }
    return dst;
}

std::string UrlDecode(const std::string &szToDecode)
{
    std::string result;
    int hex = 0;
    for (size_t i = 0; i < szToDecode.length(); ++i)
    {
        switch (szToDecode[i])
        {
        case '+':
            result += ' ';
            break;
        case '%':
            if (isxdigit(szToDecode[i + 1]) && isxdigit(szToDecode[i + 2]))
            {
                std::string hexStr = szToDecode.substr(i + 1, 2);
                hex = strtol(hexStr.c_str(), 0, 16);
                // 字母和数字[0-9a-zA-Z]、一些特殊符号[$-_.+!*'(),] 、以及某些保留字[$&+,/:;=?@]
                // 可以不经过编码直接用于URL
                if (!((hex >= 48 && hex <= 57) ||  // 0-9
                      (hex >= 97 && hex <= 122) || // a-z
                      (hex >= 65 && hex <= 90) ||  // A-Z
                      // 一些特殊符号及保留字[$-_.+!*'(),]  [$&+,/:;=?@]
                      hex == 0x21 || hex == 0x24 || hex == 0x26 || hex == 0x27 || hex == 0x28 || hex == 0x29 || hex == 0x2a || hex == 0x2b || hex == 0x2c || hex == 0x2d || hex == 0x2e || hex == 0x2f || hex == 0x3A || hex == 0x3B || hex == 0x3D || hex == 0x3f || hex == 0x40 || hex == 0x5f))
                {
                    result += char(hex);
                    i += 2;
                }
                else
                    result += '%';
            }
            else
            {
                result += '%';
            }
            break;
        default:
            result += szToDecode[i];
            break;
        }
    }
    return result;
}
bool GetQueryString(std::string &query)
{
    std::string method = getenv("METHOD");
    // std::cerr << "method: " + method << std::endl;
    if (method == "GET")
    {
        query = getenv("QUERY_STRING");
        //  cerr << "query_string: " + query << endl;
        return true;
    }
    else if (method == "POST")
    {
        int content_length = atoi(getenv("CONTENT_LENGTH"));
        // std::cerr << "debug: content_length:" << content_length << std::endl;
        char ch;
        while (content_length)
        {
            read(0, &ch, 1);
            query += ch;
            content_length--;
        }
        std::cerr << "query_string: " + query << std::endl;
        return true;
    }
    else
    {
        return false;
    }
}

void CutString(const std::string &in, const std::string sep, std::string &out1, std::string &out2)
{
    size_t pos = in.find(sep);
    if (pos != std::string::npos)
    {
        out1 = in.substr(0, pos);
        out2 = in.substr(pos + sep.size());
        size_t pos2 = out2.find(sep);
        out2 = out2.substr(0, pos2);
    }
}

bool implement(std::string &sql)
{
    MYSQL *mfp = mysql_init(nullptr);
    // 设置编码格式
    mysql_set_character_set(mfp, "utf8");
    if (mysql_real_connect(mfp, "127.0.0.1", "http_server", "llyscysygr", "http_server", 3306, nullptr, 0) == nullptr)
    {
        std::cerr << "connect mysql error!" << std::endl;
        return false;
    }
    std::cerr << "connect mysql success!" << std::endl;
    int ret = mysql_query(mfp, sql.c_str());
    // std::cout << ret << std::endl;
    mysql_close(mfp);
    return ret == 0;
}

bool Insert(std::string &name, std::string &value)
{
    std::string sql;
    sql = "insert into user values('" + name + "','" + value + "');";
    // std::cerr << sql << std::endl;
    return implement(sql);
}

int Select(std::string &name, std::string &pwd)
{
    std::string sql;
    sql = "select * from user where name='" + name + "';";
    // sql = "select * from user";
    std::cerr << sql << std::endl;
    MYSQL *mfp = mysql_init(nullptr);
    // 设置编码格式
    mysql_set_character_set(mfp, "utf8");
    if (mysql_real_connect(mfp, "127.0.0.1", "http_server", "llyscysygr", "http_server", 3306, nullptr, 0) == nullptr)
    {
        std::cerr << "connect mysql error!" << std::endl;
        return -1;
    }
    if (mysql_query(mfp, sql.c_str()))
    {
        return -1;
    }
    // 获取查询结果
    MYSQL_RES *sqlres = mysql_store_result(mfp);
    if (sqlres == nullptr)
        return 1;
    my_ulonglong num = mysql_num_rows(sqlres);
    // uint32_t col = mysql_num_fields(sqlres);
    // std::cerr << col << std::endl;
    // std::cerr << num << std::endl;
    if (num == 0)
    {
        // 账户不存在
        return 1;
    }
    MYSQL_ROW line = mysql_fetch_row(sqlres);
    std::cerr << line[0] << ":" << line[1] << std::endl;
    // std::cerr << pwd << std::endl;
    if (line[1] != pwd)
    {
        // 密码不正确
        return 2;
    }
    mysql_close(mfp);
    mysql_free_result(sqlres);
    return 0;
}
bool Compare(std::string s1, std::string s2)
{
    int size1 = s1.size(), size2 = s2.size();
    if (size1 != size2)
        return false;
    for (int i = 0; i < size1; ++i)
    {
        if (s1[i] != s2[i])
            return false;
    }
    return true;
}
// #define DEBUG
int main()
{
    std::string query;
//  query = "name=12&password=123&page=signup";
// std::string query = "name=%E9%BB%8E%E6%98%8E&password=123";
#ifndef DEBUG
    if (!GetQueryString(query))
    {
        exit(7);
    }
#endif
    std::string name;
    std::string password;
    std::string page;
    std::string pwd_page;
    CutString(query, "&", name, pwd_page);
    CutString(pwd_page, "&", password, page);
    if (name == "")
    {
        return 0;
    }
    // std::cerr << name << std::endl;
    // std::cerr << password << std::endl;

    std::string name_key;
    std::string name_value;
    CutString(name, "=", name_key, name_value);
    // std::cerr << name_key << std::endl;
    // std::cerr << name_value << std::endl;

    std::string pwd_key;
    std::string pwd_value;
    CutString(password, "=", pwd_key, pwd_value);

    std::string page_key;
    std::string page_value;
    CutString(page, "=", page_key, page_value);
    // std::cerr << page_value << std::endl;
    name_value = UrlDecode(name_value);
    pwd_value = UrlDecode(pwd_value);
    page_value = UrlDecode(page_value);
    // std::cerr << page_value << std::endl;
    // std::cerr << name + " " + password + " " + page << std::endl;

    bool res = Compare(std::string("1"), page_value);
    std::cerr << res << std::endl;
    // if (strcmp(page_value.c_str(), "signup") == 0||page_value == "signup"){
    if (res)
    {
        // 查询
        int ret = Select(name_value, pwd_value);
        if (ret == 0)
        {
            // 成功
            struct stat st;
            stat("/home/wxj/code/http-project/wwwroot/calc.html", &st);
            int fd = open("/home/wxj/code/http-project/wwwroot/calc.html", O_RDONLY);
            std::cerr << "fd:" << fd << std::endl;
            sendfile(1, fd, nullptr, st.st_size);
            std::cerr << "用户：" + name_value + " 登录成功" << std::endl;
            close(fd);
        }
        else if (ret == 1)
        {
            // 账号不存在
            std::cout << "<head><meta charset=\"UTF-8\"></head>" << std::endl;
            std::cout << "<body><h1>账号不存在</h1></body>" << std::endl;
        }
        else if (ret == 2)
        {
            // 密码错误
            std::cout << "<head><meta charset=\"UTF-8\"></head>" << std::endl;
            std::cout << "<body><h1>密码错误</h1></body>" << std::endl;
        }
    }
    else
    {
        if (Insert(name_value, pwd_value))
        {
            std::cerr << "用户：" + name_value + " " + "密码：" + pwd_value + " 注册成功" << std::endl;
            struct stat st;
            stat("/home/wxj/code/http-project/wwwroot/index.html", &st);
            int fd = open("/home/wxj/code/http-project/wwwroot/index.html", O_RDONLY);
            sendfile(1, fd, nullptr, st.st_size);
            close(fd);
        }
        else
        {
            // 用户已经存在
            std::cout << "<head><meta charset=\"UTF-8\"></head>" << std::endl;
            std::cout << "<body><h1>该用户已经存在</h1></body>" << std::endl;
        }
    }
    return 0;
}
