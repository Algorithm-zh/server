#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <pthread.h>

pthread_mutex_t mutex;

typedef struct fdinfo
{
    int fd;
    int *maxfd;
    fd_set *redset;
} FDInfo;

void *acceptConn(void *arg)
{
    printf("子线程线程ID: %ld\n",pthread_self());
    FDInfo *info = (FDInfo *)arg;
    int cfd = accept(info->fd, NULL, NULL);
    pthread_mutex_lock(&mutex);
    FD_SET(cfd, info->redset);
    *info->maxfd = cfd > *info->maxfd ? cfd : *info->maxfd;
    pthread_mutex_unlock(&mutex);

    free(info);
    return NULL;
}

void *communciation(void *arg)
{

    char buf[1024];
    FDInfo *info = (FDInfo *)arg;
    int len = read(info->fd, buf, sizeof(buf));
    if (len == -1)
    {
        perror("read error");
        free(info);
        return NULL;
    }
    else if (len > 0)
    {
        printf("client say: %s\n", buf);
        for (int i = 0; i < len; i++)
        {
            buf[i] = toupper(buf[i]);
        }
        printf("server say: %s\n", buf);
        int ret = send(info->fd, buf, strlen(buf) + 1, 0);
        if (ret == -1)
        {
            perror("send error");
            exit(-1);
        }
        return NULL;
    }
    else if (len == 0)
    {
        printf("client exit\n");
        pthread_mutex_lock(&mutex);
        FD_CLR(info->fd, info->redset);
        pthread_mutex_unlock(&mutex);
        close(info->fd);
        return NULL;
    }
    free(info);
}

// server
int main(int argc, char *argv[])
{
    pthread_mutex_init(&mutex, NULL);
    // 创建监听套接字
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1)
    {
        perror("socket error");
        exit(-1);
    };

    // 绑定
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(9999);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    // 绑定端口
    int ret = bind(lfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (ret == -1)
    {
        perror("bind error");
        exit(-1);
    }

    // 监听
    ret = listen(lfd, 128);
    if (ret == -1)
    {
        perror("listen error");
        exit(-1);
    }

    fd_set redset;
    FD_ZERO(&redset);
    FD_SET(lfd, &redset);

    int maxfd = lfd;
    while (1)
    {
        pthread_mutex_lock(&mutex);
        fd_set tmp = redset;
        pthread_mutex_unlock(&mutex);
        int ret = select(maxfd + 1, &tmp, NULL, NULL, NULL);
        //int select(int nfds, fd_set *readfds, fd_set *writefds,fd_set *exceptfds, struct timeval * timeout);
        //nfds三个集合中最大的文件描述符+1 
        // 判断是不是监听的fd
        if (FD_ISSET(lfd, &tmp))
        {
            // 接受客户端的连接
            // 创建子线程
            pthread_t tid;
            FDInfo *info = malloc(sizeof(FDInfo));
            info->fd = lfd;
            info->maxfd = &maxfd;
            info->redset = &redset;
            pthread_create(&tid, NULL, acceptConn, info);
            pthread_detach(tid);
        }
        for (int i = 0; i <= maxfd; i++)
        {
            if (i != lfd && FD_ISSET(i, &tmp))
            {
                // 接受客户端发送的数据
                pthread_t tid;
                FDInfo *info = malloc(sizeof(FDInfo));
                info->fd = i;
                info->redset = &redset;
                pthread_create(&tid, NULL, communciation, info);
                pthread_detach(tid);
            }
        }
    }

    pthread_mutex_destroy(&mutex);
    close(lfd);

    return 0;
}