/*********************************************************************************************
alarm函数定时通过管道pipefd[1]发送信号，内核接受信号，中断进程，调用信号处理函数，处理完成后返回中断处继续执行
信号处理函数执行时会屏蔽该信号的再次出发，所以信号处理函数仅仅用管道来通知主循环，让主循环来处理信号

统一事件源，将主循环的接受管道pipefd[0]也注册进epoll，利用io复用作统一处理,主循环判断pipefd[0]上的时间处理信号
************************************************************************************************/

#ifndef TIMER_LST_H
#define TIMER_LST_H

#include <unistd.h>
#include <signal.h>//alarm定时发送信号实现定时器，struct sigaction结构体定义信号处理方式
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>//fcntl系统调用可以用来对已打开的文件描述符进行各种控制操作以改变已打开文件的的各种属性
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>

#include <time.h>
#include "log.h"
#include "http_conn.h"

class util_timer;

//连接资源-->找到绑定的定时器
struct client_data{
    sockaddr_in address;
    int sockfd;
    util_timer *timer;
};

//定时器工具类-->找到绑定的客户资源
class util_timer{
    public:
    util_timer():prev(nullptr),next(nullptr){}

    public:
    time_t overtime;
    //回调函数
    void (*cb_func)(client_data *); 
    util_timer* prev,*next;
    client_data* user_data;
};

//定时器容器类
class timer_lst{
    public:
    timer_lst();
    ~timer_lst();

    void add_timer(util_timer* timer);
    void adjust_timer(util_timer* timer);
    void del_timer(util_timer* timer);
    void tick();//一次滴答，遍历处理超时任务

    private:
    void add_timer(util_timer* timer,util_timer* head);

    util_timer* head;
    util_timer* tail;

};

//封装定时器工具类，用于统一时间源
class Utils{
public:
    Utils() {}
    ~Utils() {}

    void init(int timeslot);

    //对文件描述符设置非阻塞
    int setnonblocking(int fd);

    //将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

    //信号处理函数，此处只向管道发送信号，减少进程中断时间
    static void sig_handler(int sig);

    //设置信号函数
    void addsig(int sig, void(handler)(int), bool restart = true);

    //定时处理任务，重新定时以不断触发SIGALRM信号
    void timer_handler();

    void show_error(int connfd, const char *info);

public:
    static int *u_pipefd;//共享管道fd
    timer_lst m_timer_lst;//定时器队列
    static int u_epollfd;//共享epfd,根结点
    int m_TIMESLOT;//信号发送间隔
};

void cb_func(client_data *user_data);//回调函数


#endif // !TIMER_LST_H