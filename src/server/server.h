#pragma once
#include <iostream>
#include <unordered_map>

#define CONNECT_PORT 8080

class CServer
{
public:
    CServer* GetInstance(); //获取服务器类对象

public:
    void Run();   // 服务器开始工作

private:
    void setNonBlocking(int fd);

private:
    CServer();
    ~CServer();
private:
    static CServer* m_pServer;

    int m_nListenfd;  // 用于监听的文件描述符

    // 保存用户的通信文件描述符以及用户名和密码
    using USERINFO = std::unordered_map<std::string, std::string>; 
    USERINFO m_umapClientInfo;
};

/*
Json 消息格式

*/