# HTTP项目

#### 介绍
该项目采用C/S模型，从零开始编写支持中小型应用的http，并结合mysql。整个项目服务器大体分为客户端建立连接，读取分析请求、处理请求、构建响应、构建响应几个部分。该服务器能够根据用户的请求返回简单的静态网页和动态网页，应对处理常见的错误请求。此外为了能够处理客户端发起的请求，在HTTP服务器提供的平台上搭建了CGI机制，CGI机制可以处理HTTP 的一些数据请求，并作出相应的处理。为了能够让项目更加完善，我在该服务器之上增加了一个登录和注册模块，结合mysql存储用户数据，并且部署了一个简单的计算器服务。

#### 主要技术
- 网络编程（TCP/IP协议, socket流式套接字，http协议）
- cgi技术
- 线程池


#### 开发环境

- Centos7.6、C/C++、vim、g++、Makefile、Postman


#### 参与贡献

1.  Fork 本仓库
2.  新建 Feat_xxx 分支
3.  提交代码
4.  新建 Pull Request


#### 特技

1.  使用 Readme\_XXX.md 来支持不同的语言，例如 Readme\_en.md, Readme\_zh.md
2.  Gitee 官方博客 [blog.gitee.com](https://blog.gitee.com)
3.  你可以 [https://gitee.com/explore](https://gitee.com/explore) 这个地址来了解 Gitee 上的优秀开源项目
4.  [GVP](https://gitee.com/gvp) 全称是 Gitee 最有价值开源项目，是综合评定出的优秀开源项目
5.  Gitee 官方提供的使用手册 [https://gitee.com/help](https://gitee.com/help)
6.  Gitee 封面人物是一档用来展示 Gitee 会员风采的栏目 [https://gitee.com/gitee-stars/](https://gitee.com/gitee-stars/)
