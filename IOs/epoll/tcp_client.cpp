#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
using namespace std;

int main(int argc, const char* argv[])
{
    //创建一个通信的套接字
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1)
    {
        perror("socket");
        exit(-1);
    }
    //连接服务器
    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(9999);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    int ret = connect(fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if(ret == -1)
    {
        perror("connect error");
        exit(-1);
    }

    //通信
    int num = 0;
    while(1)
    {
        //从终端读一个字符串
        char buf[1024] = {0};
        sprintf(buf, "hello, i am client %d", num++);
        write(fd, buf, strlen(buf));
        
        int len = read(fd, buf, sizeof(buf));
        if(len == -1)
        {
            perror("read");
            exit(-1);
        }
        cout << "server echo : " << buf << endl;
        sleep(1);
    }
    close(fd);

    return 0;

}