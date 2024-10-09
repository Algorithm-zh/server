#include <iostream>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <thread>
#include <mutex>
using namespace std;

mutex mtx;
void acceptConn(int curfd, int epfd)
{
    // 建立新的连接
    int cfd = accept(curfd, NULL, NULL); // 监听的文件描述符，客户端的ip和端口信息，存储addr的内存大小
    // ET模式
    // 将文件描述符设置为非阻塞
    // 得到文件描述符的属性

    int flag = fcntl(cfd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(cfd, F_SETFL, flag);
    // 新得到的文件描述符添加到epoll模型中, 下一轮循环的时候就可以被检测了
    // 通信的文件描述符检测读缓冲区数据的时候设置为边沿模式
    
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET; // 读缓冲区是否有数据
    ev.data.fd = cfd;
    mtx.lock();
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
    mtx.unlock();
    if (ret == -1)
    {
        perror("epoll_ctl-accept-error");
        exit(0);
    }

}
void communciation(int curfd, int epfd)
{
    // 处理通信的文件描述符
    // 接收数据
    char buf[5];
    memset(buf, 0, sizeof(buf));
    // 循环读数据
    while (1)
    {
        int len = recv(curfd, buf, sizeof(buf), 0);
        if (len == 0)
        {
            // 非阻塞模式下和阻塞模式是一样的 => 判断对方是否断开连接
            cout << " 客户端断开了连接." << endl;
            // 将这个文件描述符从epoll模型中删除
            mtx.lock();
            epoll_ctl(epfd, EPOLL_CTL_DEL, curfd, NULL);
            mtx.unlock();
            close(curfd);
            break;
        }
        else if (len > 0)
        {
            // 通信
            // 接收的数据打印到终端
            write(STDOUT_FILENO, buf, len);
            // 发送数据
            send(curfd, buf, len, 0);
        }
        else
        {
            // len == -1
            if (errno == EAGAIN)
            {
                cout << "数据读完了" << endl;
                break;
            }
            else
            {
                perror("recv-error");
                exit(0);
            }
        }
    }
}

// server
int main(int argc, const char *argv[])
{

    // 创建监听的套接字
    int lfd = socket(AF_INET, SOCK_STREAM, 0); // ipv4 流式传输
    if (lfd == -1)
    {
        perror("socket error");
        exit(1);
    }

    // 设置服务器的端口和ip
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(9999);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 本地多有的ＩＰ
    // 127.0.0.1
    // inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr.s_addr);

    // 设置端口复用
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 将文件描述符和本地的IP与端口进行绑定
    int ret = bind(lfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (ret == -1)
    {
        perror("bind error");
        exit(1);
    }

    // 监听
    ret = listen(lfd, 64);
    if (ret == -1)
    {
        perror("listen error");
        exit(1);
    }

    // 现在只有监听的文件描述符
    // 所有的文件描述符对应读写缓冲区状态都是委托内核进行检测的epoll
    // 创建一个epoll模型
    int epfd = epoll_create(100);
    if (epfd == -1)
    {
        perror("epoll_create");
        exit(0);
    }

    // 往epoll实例中添加需要检测的节点, 现在只有监听的文件描述符
    struct epoll_event ev;
    ev.events = EPOLLIN; // 检测lfd读读缓冲区是否有数据
    ev.data.fd = lfd;
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
    if (ret == -1)
    {
        perror("epoll_ctl");
        exit(0);
    }

    struct epoll_event evs[1024];                        // evs作为传出参数存储已经就绪的文件描述符的信息
    int size = sizeof(evs) / sizeof(struct epoll_event); // 结构体数组的容量
    // 持续检测
    while (1)
    {
        // 调用一次, 检测一次
        mtx.lock();
        int num = epoll_wait(epfd, evs, size, -1); // timeout=-1表示函数一直阻塞
        cout << "======" << num << endl;           // 打印出检测到的文件描述符个数
        mtx.unlock();

        for (int i = 0; i < num; ++i)
        {
            // 取出当前的文件描述符
            mtx.lock(); 
            int curfd = evs[i].data.fd;
            mtx.unlock();
            // 判断这个文件描述符是不是用于监听的
            if (curfd == lfd)
            {
                //监听
                thread t1(acceptConn, curfd, epfd);
                t1.detach();
            }
            else
            {
                //通信
                thread t2(communciation, curfd, epfd);
                t2.detach();
            }
        }
    }

    return 0;
}
