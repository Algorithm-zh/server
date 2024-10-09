#include "ThreadPool.h"
#include <string.h>
#include <unistd.h>
//包含了sys/socket.h
#include <arpa/inet.h>
#include <thread>
#include <iostream>
#include <functional>

using namespace std;
void working(struct sockaddr_in caddr, int cfd);
void acceptConn(int fd, ThreadPool& pool);
int main()
{
    //1，创建监听的套接字
    int fd = socket(AF_INET, SOCK_STREAM, 0);//ipv4 流式协议 tcp
    if(fd == -1)
    {
        perror("socket");
        return -1;
    }

    //2.绑定本地的ip port
    struct sockaddr_in saddr;//主要存储ip和端口信息，绑定的时候必须是大端的，即网络字节序
    saddr.sin_family = AF_INET;//指定ipv4
    saddr.sin_port = htons(9999);//转换为大端，端口自己指定
    saddr.sin_addr.s_addr = INADDR_ANY;//0.0.0.0,自动读网卡实际ip地址，自动绑定实际ip地址
    int ret = bind(fd, (struct sockaddr*)&saddr, sizeof(saddr));
    if(ret == -1)
    {
        perror("bind");
        return -1;
    }
    //3.设置监听
    ret = listen(fd, 128);//设置能最多能监听到128个连接请求
    if(ret == -1)
    {
        perror("listen");
        return -1;
    }

    //创建线程池
    ThreadPool pool(8);
    //添加建立连接的任务
    // auto func = bind(acceptConn, fd, pool);
    // auto func = bind(acceptConn, fd, pool);
    pool.addTask([&]() { acceptConn(fd, pool); });
    // pool.addTask(func);
    getchar();

    return 0;
}

void acceptConn(int fd, ThreadPool& pool)
{
    //4.阻塞等待客户端连接
    struct sockaddr_in caddr;
    unsigned int addrlen = sizeof(struct sockaddr_in);
    while(1){
        int cfd = accept(fd, (struct sockaddr*)&caddr, &addrlen);
        cout << "accept a new client..." << endl;
        if(cfd == -1)
        {
            perror("accept");
            break;
        }
        //添加通信的任务
        // pool.addTask(bind(working, caddr, cfd));
        cout << "add working task" << endl;
        pool.addTask([&]() { working(caddr, cfd); });
    }
    close(fd);//关闭监听的描述符
    
}

void working(struct sockaddr_in caddr, int cfd)
{
    cout << "6666" << endl;
    //连接建立成功，打印客户端的ip和端口信息
    char ip[32];
    //大端转换为小端
    cout << "client ip = " 
    << inet_ntoa(caddr.sin_addr) //整型的网络字节序转化为字符串的主机字节序
    << ", port = " << ntohs(caddr.sin_port) << endl;
    //5.通信
    while(1)
    {
        //接受数据
        char buf[1024];
        int len = recv(cfd, buf, sizeof(buf), 0);
        if(len > 0)
        {
            cout << "server  recv buf = " <<  buf << endl;
            send(cfd, buf, len, 0);
        }
        else if(len == 0)
        {
            cout << "client closed..." << endl;
            break;
        }
        else
        {
            perror("recv");
            break;
        }
    }
    //6.关闭文件描述符
    close(cfd);//通信的

}
