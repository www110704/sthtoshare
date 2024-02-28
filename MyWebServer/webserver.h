#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "threadpool.h"
#include "http_conn.h"

const int MAX_FD = 65536;           //最大文件描述符
const int MAX_EVENT_NUMBER = 10000; //最大事件数
const int TIMESLOT = 5;             //最小超时单位

class WebServer{
public:
    WebServer();
    ~WebServer();

    void init(int port , string user, string passWord, string databaseName,
              int log_write , int opt_linger, int trigmode, int sql_num,
              int thread_num, int close_log, int actor_model);

    void thread_pool();//创建线程池
    void sql_pool();//创建sql连接池
    void log_write();//初始化日志（）
    void trig_mode();//设置fd和http的ET LT
    
    void init_client(int connfd, struct sockaddr_in client_address);//初始化http,套接字绑定定时器，然后加入定时器链表
    void adjust_timer(util_timer *timer);//修改定时器顺序
    void del_timer(util_timer *timer, int sockfd);//删除定时器并关闭连接

    void eventListen();//网络编程基础步骤，创建epoll根结点，创建管道，设置信号处理函数

    //eventLoop()调用处理各种就绪事件
    bool dealclinetdata();
    bool dealwithsignal(bool& timeout, bool& stop_server);
    void dealwithread(int sockfd);
    void dealwithwrite(int sockfd);

    void eventLoop();//主循环

public:
    //基础
    int m_port;//端口号
    char *m_root;//资源文件根目录
    int m_log_write;//异步日志
    int m_close_log_flag;//关闭日志
    int m_actormodel;//Preactor or Reactor

    //管道
    int m_pipefd[2];
    int m_epollfd;
    http_conn *users;//http数组

    //数据库相关
    connection_pool *m_connPool;
    string m_user;         //登陆数据库用户名
    string m_passWord;     //登陆数据库密码
    string m_databaseName; //使用数据库名
    int m_sql_num;

    //线程池相关
    threadpool<http_conn> *m_pool;
    int m_thread_num;

    //epoll_event相关
    epoll_event events[MAX_EVENT_NUMBER];//内核事件表

    int m_listenfd;
    int m_OPT_LINGER;//连接关闭方式：0 close立即返回，1 close延迟直到超时或发送完
    int m_TRIGMode;
    int m_LISTENTrigmode;//fd监听触发模式--sockfd--add/mod/delfd()
    int m_CONNTrigmode;//http连接读触发模式--read_once()

    //定时器相关
    client_data *users_timer;//用户信息，包含用户addr，sockfd，定时器指针
    Utils utils;//定时器封装类统一事件源，包含管道，epfd，定时器链表，定时间隙等

};


#endif