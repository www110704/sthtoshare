#include"webserver.h"

WebServer::WebServer(){
    users = new http_conn[MAX_FD];
    users_timer = new client_data[MAX_FD];//每个fd一个定时器

    //设置资源路径
    char server_path[200];
    //获取当前工作目录的绝对路径，与getcurdir()函数相比，它得到的路径包含盘符
    getcwd(server_path, 200);
    char root[6] = "/root";
    m_root = (char *)malloc(strlen(server_path) + strlen(root) + 1);
    strcpy(m_root, server_path);
    strcat(m_root, root);
}

WebServer::~WebServer(){//关闭fd，释放指针，数组
    close(m_epollfd);
    close(m_listenfd);
    close(m_pipefd[1]);
    close(m_pipefd[0]);
    delete[] users;
    delete[] users_timer;
    delete m_pool;
    free(m_root);
}

void WebServer::init(int port , string user, string passWord, string databaseName,
              int log_write , int opt_linger, int trigmode, int sql_num,
              int thread_num, int close_log, int actor_model)
{
    m_port = port;
    m_user = user;
    m_passWord = passWord;
    m_databaseName = databaseName;
    m_sql_num = sql_num;
    m_thread_num = thread_num;
    m_log_write = log_write;
    m_OPT_LINGER = opt_linger;
    m_TRIGMode = trigmode;
    m_close_log_flag = close_log;
    m_actormodel = actor_model;
}

void WebServer::sql_pool()//创建sql连接池
{
    m_connPool = connection_pool::GetInstance();//单列模式唯一实例
    m_connPool->init("localhost", m_user, m_passWord, m_databaseName, 3306, m_sql_num, m_close_log_flag);
    users->initmysql_result(m_connPool);//初始化数据库用户表（全局表），其实这个map可以在任意地方
}

void WebServer::thread_pool()//创建线程池
{
    m_pool = new threadpool<http_conn>(m_actormodel,m_connPool,m_thread_num);
}

void WebServer::log_write()//初始化日志（）
{
    if(m_close_log_flag == 0){
        if(m_log_write==1){
            Log::get_instance()->init("./ServerLog", m_close_log_flag, 2000, 800000, 800);
        }
        else{
            Log::get_instance()->init("./ServerLog", m_close_log_flag, 2000, 800000, 0);
        }
    }
}

void WebServer::trig_mode()//设置fd和http的ET LT
{
    //LT + LT
    if(m_TRIGMode==0){
        m_LISTENTrigmode = 0;
        m_CONNTrigmode = 0;
    }
    //LT + ET
    else if(m_TRIGMode==1){
        m_LISTENTrigmode = 0;
        m_CONNTrigmode = 1;
    }
    //ET + LT
    else if(m_TRIGMode==2){
        m_LISTENTrigmode = 1;
        m_CONNTrigmode = 0;
    }
    //ET + ET
    else if(m_TRIGMode==3){
        m_LISTENTrigmode = 1;
        m_CONNTrigmode = 1;
    }
}

//初始化http,用户信息+定时器，加入utils链表
void WebServer::init_client(int connfd, struct sockaddr_in client_address)//套接字绑定定时器，然后加入定时器链表
{
    users[connfd].init(connfd, client_address, m_root, m_CONNTrigmode, m_close_log_flag, m_user, m_passWord, m_databaseName);
    //初始化client_data数据
    //创建定时器，设置回调函数和超时时间，绑定用户数据，将定时器添加到链表中
    users_timer[connfd].address = client_address;
    users_timer[connfd].sockfd = connfd;
    util_timer *timer = new util_timer;//new定时器，和client_data相互包含
    timer->user_data = &users_timer[connfd];
    timer->cb_func = cb_func;//lst_timer.h中的函数
    time_t cur = time(NULL);
    timer->overtime = cur + 3 * TIMESLOT;//三个间隙的超时时间
    users_timer[connfd].timer = timer;
    utils.m_timer_lst.add_timer(timer);//加入链表
}

//调用utils->lst->adjust_timer
void WebServer::adjust_timer(util_timer *timer)//修改定时器顺序
{
    time_t cur = time(NULL);
    timer->overtime = cur + 3 * TIMESLOT;
    utils.m_timer_lst.adjust_timer(timer);
    LOG_INFO("%s", "adjust timer once");
}

void WebServer::del_timer(util_timer *timer, int sockfd)//删除定时器并关闭连接
{
    timer->cb_func(&users_timer[sockfd]);//回调函数关闭连接
    if (timer)
    {
        utils.m_timer_lst.del_timer(timer);//删除链表中的定时器
    }
    LOG_INFO("close fd %d", users_timer[sockfd].sockfd);
}

void WebServer::eventListen()//网络编程基础步骤，创建epoll根结点，创建管道，设置信号处理函数
{
    //网络编程基础
    m_listenfd = socket(AF_INET,SOCK_STREAM,0);//BSD是AF_INET，POSIX是PF
    assert(m_listenfd>=0);

    //设置关闭方式
    if (0 == m_OPT_LINGER)
    {
        struct linger tmp = {0, 1};//close立即返回
        setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    }
    else if (1 == m_OPT_LINGER)//优雅关闭连接
    {
        struct linger tmp = {1, 1};//close延迟返回
        setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    }

    utils.init(TIMESLOT);//设置定时器定时间隙

    //设置用于绑定的地址
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    //字节序转换
    address.sin_addr.s_addr = htonl(INADDR_ANY);//host to network long
    address.sin_port = htons(m_port);//host to network shot

    int flag = 1;
    //SO_REUSEADDR可实现地址复用---复用TIME_WAIT状态且地址端口相同的套接字
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    int ret = bind(m_listenfd, (struct sockaddr *)&address, sizeof(address));
    assert(ret >= 0);//断定，c++宏，条件不成立，程序会终止
    ret = listen(m_listenfd, 5);
    assert(ret >= 0);

    //epoll创建内核事件表
    epoll_event events[MAX_EVENT_NUMBER];
    m_epollfd = epoll_create(5);
    assert(m_epollfd != -1);
	////将listenfd放在epoll树上
    utils.addfd(m_epollfd, m_listenfd, false, m_LISTENTrigmode);
    
	//将上述epollfd赋值给http类对象的m_epollfd属性
    http_conn::m_epollfd = m_epollfd;

    //创建管道套接字
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipefd);
    assert(ret != -1);
	//设置管道写端为非阻塞，为什么写端要非阻塞？
    utils.setnonblocking(m_pipefd[1]);
	//设置管道读端为ET非阻塞
    utils.addfd(m_epollfd, m_pipefd[0], false, 0);

    //当往一个写端关闭的管道或socket连接中连续写入数据时会引发SIGPIPE信号,默认行为是结束进程，所以设置忽略
    utils.addsig(SIGPIPE, SIG_IGN);/* Ignore signal.  */
	//传递给主循环的信号值，这里只关注SIGALRM和SIGTERM
    utils.addsig(SIGALRM, utils.sig_handler, false);//sig_handler只是向管道发送信息，由主循环接收处理dealwithsignal()
    utils.addsig(SIGTERM, utils.sig_handler, false);
	
	//每隔TIMESLOT时间触发SIGALRM信号
    alarm(TIMESLOT);

    //静态套接字和epoll
    Utils::u_pipefd = m_pipefd;
    Utils::u_epollfd = m_epollfd;

}

//处理客户信息的函数
//ET or LT  accetp函数取出客户连接fd
bool WebServer::dealclinetdata()
{
    struct sockaddr_in client_address;
    socklen_t addr_len = sizeof(client_address);
    if(m_LISTENTrigmode==0){//LT模式
        int connfd = accept(m_listenfd,(struct sockaddr*)&client_address,&addr_len);
        if (connfd < 0)
        {
            LOG_ERROR("%s:errno is:%d", "accept error", errno);
            return false;
        }
        if (http_conn::m_user_count >= MAX_FD)
        {
            utils.show_error(connfd, "Internal server busy");
            LOG_ERROR("%s", "Internal server busy");
            return false;
        }
        init_client(connfd, client_address);//初始化http,用户信息+定时器，加入utils链表
    }
    else
    {
        while (1)//循环一次读完
        {
            int connfd = accept(m_listenfd, (struct sockaddr *)&client_address, &addr_len);
            if (connfd < 0)
            {
                LOG_ERROR("%s:errno is:%d", "accept error", errno);
                break;
            }
            if (http_conn::m_user_count >= MAX_FD)
            {
                utils.show_error(connfd, "Internal server busy");
                LOG_ERROR("%s", "Internal server busy");
                break;
            }
            init_client(connfd, client_address);//设置好accept用户的定时器
        }
        return false;
    }
    return true;
}

//recv接收信号，可能积累了几个信号，循环处理，主要就SIGALRM和SIGTERM
bool WebServer::dealwithsignal(bool& timeout, bool& stop_server)
{
    int ret = 0;
    int sig;
    char signals[1024];
    ret = recv(m_pipefd[0], signals, sizeof(signals), 0);//返回的是数据长度？
    if (ret == -1)
    {
        return false;
    }
    else if (ret == 0)
    {
        return false;
    }
    else
    {
        for (int i = 0; i < ret; ++i)
        {
            switch (signals[i])
            {
            case SIGALRM:
            {
                timeout = true;
                break;
            }
            case SIGTERM:
            {
                stop_server = true;
                break;
            }
            }
        }
    }
    return true;
}

void WebServer::dealwithread(int sockfd){
    util_timer *timer = users_timer[sockfd].timer;
    //分Preactor 和 Reactor
    if(m_actormodel==1){//Reactor
        if (timer)
        {
            adjust_timer(timer);//调整定时器队列顺序
        }

        //交给工作线程io+逻辑处理
        m_pool->append(users + sockfd, 0);//0为读，1为写
        while (true)//等待工作线程完成读improv = 1？
        {
            if (1 == users[sockfd].improv)
            {
                if (1 == users[sockfd].timer_flag)//timer_flag=1 没读成功？
                {
                    del_timer(timer, sockfd);//cb_func回调函数服务器关闭连接
                    users[sockfd].timer_flag = 0;
                }
                users[sockfd].improv = 0;//重置
                break;
            }
        }

    }
    else{//Preactor
        if (users[sockfd].read_once())//主线程io
        {
            LOG_INFO("recv the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));//转换网络地址格式
            //若监测到读事件，将该事件放入请求队列
            m_pool->append_p(users + sockfd);
            if (timer)
            {
                adjust_timer(timer);
            }
        }
        else
        {
            LOG_INFO("Preactor read_once fail");
            del_timer(timer, sockfd);//cb_func回调函数服务器关闭连接
        }
    }
}

void WebServer::dealwithwrite(int sockfd){
    util_timer *timer = users_timer[sockfd].timer;
    //reactor
    if (1 == m_actormodel)
    {
        if (timer)
        {
            adjust_timer(timer);
        }

        m_pool->append(users + sockfd, 1);//1为写事件

        while (true)
        {
            if (1 == users[sockfd].improv)
            {
                if (1 == users[sockfd].timer_flag)
                {
                    del_timer(timer, sockfd);//cb_func回调函数服务器关闭连接
                    users[sockfd].timer_flag = 0;
                }
                users[sockfd].improv = 0;
                break;
            }
        }
    }
    else
    {
        //proactor
        if (users[sockfd].write())
        {
            LOG_INFO("send data to the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));

            if (timer)
            {
                adjust_timer(timer);
            }
        }
        else
        {
            del_timer(timer, sockfd);//cb_func回调函数服务器关闭连接
        }
    }
}

//主循环
void WebServer::eventLoop()
{
    //dealwithsignal参数
    bool timeout = false;
    bool stop_server = false;
    while (!stop_server){
        //epoll_wait()最后一个参数>0为阻塞时间,0立即返回，-1则无限期阻塞
        int ready_num = epoll_wait(m_epollfd,events,MAX_EVENT_NUMBER,-1);//无限期阻塞
        if (ready_num < 0 && errno != EINTR)
        {
            LOG_ERROR("%s", "epoll failure");
            break;
        }
        //events拷贝了就绪事件
        for (int i = 0; i < ready_num; i++)
        {
            int sockfd = events[i].data.fd;
            if (sockfd == m_listenfd){
                bool flag = dealclinetdata();//封装处理客户信息函数，调用acctpt取出就绪连接
                if (flag==false)
                    continue;
            }
            else if(sockfd==m_pipefd[0]&&(events[i].events & EPOLLIN)){//addfd注册了EPOLLIN
                bool flag = dealwithsignal(timeout, stop_server);//调用recv读管道，再改变传入的timeout和stop_server
                if (false == flag)
                    LOG_ERROR("%s", "dealclientdata failure");
            }
            else if(events[i].events & EPOLLIN){//正常读事件
                dealwithread(sockfd);
            }
            else if(events[i].events & EPOLLOUT){//写事件
                dealwithwrite(sockfd);
            }
            //分别为对端关闭(写)连接，连接关闭,发生错误
            else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){//异常事件
                //服务器端关闭连接，移除对应的定时器
                util_timer *timer = users_timer[sockfd].timer;
                LOG_INFO("event error: EPOLLRDHUP | EPOLLHUP | EPOLLERR");
                del_timer(users_timer[sockfd].timer, sockfd);//调用cb_func回调函数关闭服务器连接和删除定时器
            }
        }

        //dealwithsignal处理了超时信号后timeout=true,开始处理超时的定时器
        if (timeout)//超时
        {
            utils.timer_handler();
            LOG_INFO("%s", "timer tick");
            timeout = false;
        }
    }
}


