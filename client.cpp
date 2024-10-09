#include <string.h>
#include <unistd.h>
//包含了sys/socket.h
#include <arpa/inet.h>
#include <iostream>
using namespace std;

int main()
{
    //1，创建通信的套接字
    int fd = socket(AF_INET, SOCK_STREAM, 0);//ipv4 流式协议 tcp
    if(fd == -1)
    {
        perror("socket");
        return -1;
    }

    //2.连接服务器的ip port
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(9999);//转换为大端，端口自己指定
    inet_pton(AF_INET, "127.0.0.1", &saddr.sin_addr.s_addr);
    int ret = connect(fd, (struct sockaddr*)&saddr, sizeof(saddr));
    if(ret == -1)
    {
        perror("connect");
        return -1;
    }

    int number = 0;
    //3.通信
    while(1)
    {
        //发送数据
        char buf[1024];
        sprintf(buf, "hello, i am client %d", number ++ );
        send(fd, buf, strlen(buf) + 1, 0);

        //接收数据
        memset(buf, 0, sizeof(buf));
        int len = recv(fd, buf, sizeof(buf), 0);
        if(len > 0)
        {
            cout << "client recv buf = " << buf << endl;
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
        sleep(1);
    }
    //6.关闭文件描述符
    close(fd);

    return 0;
}