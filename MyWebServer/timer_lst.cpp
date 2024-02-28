#include "timer_lst.h"

timer_lst::timer_lst()
{
    head = nullptr;
    tail = nullptr;
}

timer_lst::~timer_lst()
{
    util_timer* tmp = head;
    while(tmp){
        head = tmp->next;
        delete tmp;
        tmp = head;
    }
}
//先处理特殊情况，再调用私有add_timer
void timer_lst::add_timer(util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    if (!head)
    {
        head = tail = timer;
        return;
    }
    if (timer->overtime < head->overtime)
    {
        timer->next = head;
        head->prev = timer;
        head = timer;
        return;
    }
    add_timer(timer, head);
}

//调整timer位置，重新调用add_timer插入
void timer_lst::adjust_timer(util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    util_timer *tmp = timer->next;
    if (!tmp || (timer->overtime < tmp->overtime))
    {
        return;
    }
    if (timer == head)
    {
        head = head->next;
        head->prev = NULL;
        timer->next = NULL;
        add_timer(timer, head);
    }
    else
    {
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        add_timer(timer, timer->next);
    }
}

//根据不同位置删除timer
void timer_lst::del_timer(util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    if ((timer == head) && (timer == tail))
    {
        delete timer;
        head = NULL;
        tail = NULL;
        return;
    }
    if (timer == head)
    {
        head = head->next;
        head->prev = NULL;
        delete timer;
        return;
    }
    if (timer == tail)
    {
        tail = tail->prev;
        tail->next = NULL;
        delete timer;
        return;
    }
    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
}

//一次滴答，定时任务处理函数,遍历处理所有超任务，并删除定时器
void timer_lst::tick()
{
    if (!head)
    {
        return;
    }
    
    time_t cur = time(NULL);
    util_timer *tmp = head;
    while (tmp)
    {
        if (cur < tmp->overtime)//后面的都没超时，break
        {
            break;
        }
        tmp->cb_func(tmp->user_data);//回调函数关闭连接
        head = tmp->next;
        if (head)
        {
            head->prev = NULL;
        }
        delete tmp;
        tmp = head;
    }
}

//私有函数，
void timer_lst::add_timer(util_timer *timer, util_timer *head)
{
    util_timer *prev = head;
    util_timer *tmp = prev->next;
    while (tmp)//遍历插入timer
    {
        if (timer->overtime < tmp->overtime)
        {
            prev->next = timer;
            timer->next = tmp;
            tmp->prev = timer;
            timer->prev = prev;
            break;
        }
        prev = tmp;
        tmp = tmp->next;
    }
    if (!tmp)//若遍历到尾部，则插在尾部
    {
        prev->next = timer;
        timer->prev = prev;
        timer->next = NULL;
        tail = timer;
    }
}


//-----------------
//Utils类
void Utils::init(int timeslot){
    this->m_TIMESLOT = timeslot;
}

//对文件描述符设置非阻塞??
int Utils::setnonblocking(int fd){
    //fcntl系统调用可以用来对已打开的文件描述符进行各种控制操作以改变已打开文件的的各种属性
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//参数：根节点,待插入节点,是否一次性事件，触发模式
void Utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode){
    epoll_event event;
    event.data.fd = fd;
    if (1 == TRIGMode)//ET边缘触发
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else//LT水平触发
        event.events = EPOLLIN | EPOLLRDHUP;

    if (one_shot)//设置单次触发模式（短连接），避免竞争文件描述符
        //用过一次后从内核就绪列表中删除，如果想要再次使用EPOLL_CTL_MOD重新添加事件
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

//信号处理函数,此处仅仅向管道发送消息，主循环接收后再处理
void Utils::sig_handler(int sig){
    //首先保留原来的errno，保证可重入性
    int save_erron = errno;
    if(send(u_pipefd[1],(char*)&sig,1,0)==-1){
        throw exception();
    }
    errno = save_erron;
}

//设置信号处理方式struct sigaction,信号，处理函数timer_handler，
void Utils::addsig(int sig, void(handler)(int), bool restart){
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));//全部置空
    sa.sa_handler = handler;
    if (restart)//信号处理行为
        sa.sa_flags |= SA_RESTART;//执行信号处理后自动重启动先前中断的系统调用
    sigfillset(&sa.sa_mask);//将所有信号加入至信号集
    assert(sigaction(sig, &sa, NULL) != -1);
}

//信号处理函数，重新定时以不断触发SIGALRM信号
void Utils::timer_handler(){
    m_timer_lst.tick();//遍历处理超时任务
    alarm(m_TIMESLOT);//重新定时来触发SIGALARM信号
}

//给用户发送错误信息
void Utils::show_error(int connfd, const char *info){
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

//static类型需要在类外初始化
int *Utils::u_pipefd = 0;//管道指针
int Utils::u_epollfd = 0;//epoll根结点


void cb_func(client_data *user_data){//传入需要删除的用户连接数据
    epoll_ctl(Utils::u_epollfd,EPOLL_CTL_DEL,user_data->sockfd,0);//删除红黑书上注册的节点
    assert(user_data);
    //关闭用户fd,用户连接数-1
    close(user_data->sockfd);
    http_conn::m_user_count--;
}