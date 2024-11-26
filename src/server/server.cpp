#include "server.h"
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <exception>
#include <thread>
#include <mutex>

CServer* CServer::m_pServer = nullptr;

CServer::CServer() : m_nListenfd(0)
{
    // 创建监听套接字
    m_nListenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(m_nListenfd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 设置套接字为非阻塞模式
    this->setNonBlocking(m_nListenfd);

    // 端口复用
    int opt = 1;
    setsockopt(m_nListenfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    // 绑定服务器的IP地址和端口号
    sockaddr_in serveraddr;
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    serveraddr.sin_port = htons(CONNECT_PORT);
    serveraddr.sin_family = AF_INET;
    if(bind(m_nListenfd, reinterpret_cast<sockaddr*>(&serveraddr), sizeof(serveraddr)) == -1)
    {
        perror("bind");
        close(m_nListenfd);
        exit(EXIT_FAILURE);
    }

    // 设置监听
    if(listen(m_nListenfd, 10) == -1)
    {
        perror("listen");
        close(m_nListenfd);
        exit(EXIT_FAILURE);
    }

    std::cout << "server listening on port " << std::endl;
}

void CServer::Run()
{
    // 创建epoll实例
    int epfd = epoll_create(1);

    epoll_event epev;
    epev.data.fd = m_nListenfd;
    epev.events = EPOLLIN;  // 
    if(epoll_ctl(epfd, EPOLL_CTL_ADD, m_nListenfd, &epev) == -1)  // 把监听的文件描述符加入到epoll
    {
        perror("epoll_ctl");
        close(m_nListenfd);
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    epoll_event epevs[1024];
    while(true)
    {
        int nfd = epoll_wait(epfd, epevs, 1024, -1);
        if(nfd == -1)
        {
            perror("epoll_wait");
            continue;
        }

        for(int i = 0; i < nfd; i++)
        {
            int curfd = epevs[i].data.fd;
            if(curfd == m_nListenfd) // 监听的文件描述符有数据到达，说明有客户端连接
            {
                // 创建用于通信的文件描述符
                sockaddr_in clientaddr;   
                socklen_t length = sizeof(clientaddr);
                int clientfd = accept(curfd, reinterpret_cast<sockaddr*>(&clientaddr), &length);
                if(clientfd == -1)
                {
                    perror("accept");
                    continue;
                }

                // 将文件描述符设置为非阻塞模式
                this->setNonBlocking(clientfd);

                // 加入到epoll数组中监听该文件描述符的数据
                epoll_event epevClient;
                epevClient.data.fd = clientfd;
                epevClient.events = EPOLLIN;
                if(epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &epevClient) == -1)
                {
                    perror("epoll_ctl");
                    continue;
                }
                
                std::cout << "连接人数 + 1"  << std::endl;
            }
            else
            {
                // 通信文件描述符有数据到达，说明有用户发送数据
                int ret = recv(curfd, buffer, 1024, 0);
                if(ret == -1)
                {
                    if(errno == EAGAIN)
                        continue;
                    perror("recv");
                    close(curfd);
                    continue;
                }
                else if (ret == 0)  // 客户端断开连接
                {
                    close(curfd);
                    continue;
                }
                else  // 有数据到达
                {
                    // 使用线程来处理，后期改成线程池
                }
            }
        }
    }
}