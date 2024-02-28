#include"http_conn.h"

//全局变量
//定义http响应的一些状态信息
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file form this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the request file.\n";

locker m_lock;//map的锁
map<string, string> users;//用户名密码


//全局函数
//epoll相关
//对文件描述符设置非阻塞
int setnonblocking(int fd){
    //fcntl系统调用可以用来对已打开的文件描述符进行各种控制操作以改变已打开文件的的各种属性
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addfd(int epollfd,int fd, bool one_shot,int TRIGMODE){
    epoll_event event;
    event.data.fd = fd;
    if(TRIGMODE==0){
        event.events = EPOLLIN | EPOLLRDHUP;//LT
    }
    else{
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;//ET
    }
    if (one_shot)
        event.events |= EPOLLONESHOT;//只监听一次事件，继续监听需要再次加入epoll监听队列
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);//向epoll添加要监听的文件，加入红黑树（内核事件表）
    setnonblocking(fd);
}

void modfd(int epollfd,int fd, int modev,int TRIGMODE){
    epoll_event event;
    event.data.fd = fd;
    if (1 == TRIGMODE)//ET
        event.events = modev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    else
        event.events = modev | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

void removefd(int epollfd,int fd){
    epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,0);
    close(fd);
}


//函数对象-------------------------------------------------------------

//静态变量初始化
int http_conn::m_user_count = 0;
int http_conn::m_epollfd = -1;

//初始化mysql连接，查询获取sql中所有用户名和密码并存入map中
void http_conn::initmysql_result(connection_pool *connPool){
    MYSQL* mysql = nullptr;
    connRAII mysqlcon(&mysql,connPool);
    //在user表中检索username，passwd数据
    if (mysql_query(mysql, "SELECT username,passwd FROM user"))
    {
        LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
    }
    //从表中检索完整的结果集
    MYSQL_RES *result = mysql_store_result(mysql);
    //返回结果集中的列数
    int num_fields = mysql_num_fields(result);
    //返回所有字段结构的数组
    MYSQL_FIELD *fields = mysql_fetch_fields(result);
    //从结果集中获取下一行，将对应的用户名和密码，存入map中
    while (MYSQL_ROW row = mysql_fetch_row(result))
    {
        string temp1(row[0]);
        string temp2(row[1]);
        users[temp1] = temp2;
    }
}

//初始化连接,外部调用初始化套接字地址
void http_conn::init(int sockfd, const sockaddr_in &addr, char *root, int TRIGMode,int close_log, string user, string passwd, string sqlname){
    m_sockfd = sockfd;
    m_address = addr;
    doc_root = root;
    m_TRIGMode = TRIGMode;
    m_close_log_flag = close_log;

    addfd(m_epollfd,sockfd,true,TRIGMode);//向epfd注册skfd
    m_user_count++;

    strcpy(sql_user, user.c_str());
    strcpy(sql_passwd, passwd.c_str());
    strcpy(sql_name, sqlname.c_str());

    init();
}

//工作线程调用处理读缓冲区和写缓冲区
void http_conn::process(){
    HTTP_CODE read_ret = process_read();
    if (read_ret == NO_REQUEST)//不完整
    {
		//重新注册并监听读事件
        modfd(m_epollfd, m_sockfd, EPOLLIN, m_TRIGMode);//将事件重置为EPOLLONESHOT
        return;
    }

    bool write_ret = process_write(read_ret);
    if(!write_ret) close_conn();
    //注册并监听写事件
    modfd(m_epollfd, m_sockfd, EPOLLOUT, m_TRIGMode);
}


//recv将套接字缓冲区中的数据写入读缓冲区，非阻塞ET工作模式下，需要一次性将数据读完
bool http_conn::read_once(){
    if (m_read_idx >= READ_BUFFER_SIZE)
    {
        return false;
    }
    int bytes_read = 0;

    //LT读取数据
    if (0 == m_TRIGMode)//触发模式为0---LT水平触发模式
    {
		//从套接字接收数据，存储在m_read_buf缓冲区
        bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
        m_read_idx += bytes_read;//read_index移动bytes_read个位置

        if (bytes_read <= 0)//出错或连接关闭
        {
            return false;
        }

        return true;
    }
    //ET读数据
    else
    {
        while (true)//不断读直到读完
        {
			//读取数据存储在m_read_buf缓冲区
            bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
            if (bytes_read == -1)//出错
            {
                //EAGAIN：这个错误表示资源暂时不够
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                return false;//其他错误返回false
            }
            else if (bytes_read == 0)//连接关闭
            {
                return false;
            }
			//读取下标向后移
            m_read_idx += bytes_read;
        }
        return true;
    }
}

//将响应报文和资源文件写入fd缓冲区,写完或错误则取消资源文件的内存映射，重新初始化http对象
//循环writev()将用户进程空间中多个缓冲m_iv中保存的数据先全部复制到套接字的发送缓冲区m_sockfd中
bool http_conn::write(){
    if (bytes_to_send == 0)
    {
        modfd(m_epollfd, m_sockfd, EPOLLIN, m_TRIGMode);
        init();//重新初始化HTTP对象
        return true;
    }
    int write_ret = 0;
    while(true){
        //writev返回已经写入的长度或者EAGAIN(errno),但是需要调用者重新处理iovec
        write_ret = writev(m_sockfd,m_iv,m_iv_count);//将fd内容写入iovec
        if(write_ret<0){//失败返回-1
            if(errno==EAGAIN){//缓冲区满了，重新注册再来
                modfd(m_epollfd,m_sockfd,EPOLLOUT,m_TRIGMode);
                return true;
            }   
            unmap();//其他错误则取消资源文件的映射
            return false;
        }
        //更新buf
        bytes_have_send += write_ret;//更新已发送字节
        bytes_to_send -= write_ret;//更新已发送字节数

        //先发送m_iv[0],再发送m_iv[1]
        //bytes_to_send = m_iv[0].iov_len+m_iv[1].iov_len,bytes_to_send
        if(bytes_have_send>=m_iv[0].iov_len){//第一个iovec头部信息的数据已发送完，发送第二个iovec数据
            m_iv[0].iov_len= 0;
            m_iv[1].iov_base = m_write_buf+(bytes_have_send-m_write_idx);
            m_iv[1].iov_len = bytes_to_send;//剩下
        }
        else{//继续发送第一个iovec头部信息的数据
            m_iv[0].iov_base = m_write_buf + bytes_have_send;
            m_iv[0].iov_len = m_iv[0].iov_len - bytes_have_send;
        }
        //发送完后
        if (bytes_to_send <= 0){
            unmap();//解除资源文件映射
            //重置EPOLLINONESHOT时间
            modfd(m_epollfd, m_sockfd, EPOLLIN, m_TRIGMode);
            //浏览器的请求为长连接
            if (m_linger)//keep_alive
            {
				//重新初始化HTTP对象
                init();
                return true;
            }
            else
            {
                return false;
            }
        }
    }
}

//关闭连接，关闭一个连接，客户总量减一
void http_conn::close_conn(bool real_close){
    if (real_close && (m_sockfd != -1))
    {
        printf("close %d\n", m_sockfd);
        removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        m_user_count--;
    }
}

//初始化http对象
void http_conn::init(){
    mysql = NULL;
    bytes_to_send = 0;
    bytes_have_send = 0;
    m_check_state = CHECK_STATE_REQUESTLINE;//主状态机状态
    m_linger = false;
    m_method = GET;
    m_url = 0;
    m_version = 0;
    m_content_length = 0;
    m_host = 0;
    m_start_line = 0;
    m_checked_idx = 0;
    m_read_idx = 0;
    m_write_idx = 0;
    cgi = 0;
    m_state = 0;
    timer_flag = 0;
    improv = 0;

    memset(m_read_buf, '\0', READ_BUFFER_SIZE);
    memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
    memset(m_real_file, '\0', FILENAME_LEN);
}

//从状态机解析一行，每行数据末尾的\r\n置为\0\0
http_conn::LINE_STATUS http_conn::parse_line(){
    LOG_DEBUG("parse_line:");
    char temp;
    for (; m_checked_idx < m_read_idx; ++m_checked_idx)
    {
        temp = m_read_buf[m_checked_idx];//读取buffer中的数据
		//为什么要判断两次？
		//一般是上次读取到\r就到了buffer末尾，没有接收完整，再次接收时会出现这种情况
        if (temp == '\r')
        {
            if ((m_checked_idx + 1) == m_read_idx)
                return LINE_OPEN;
            else if (m_read_buf[m_checked_idx + 1] == '\n')
            {
                m_read_buf[m_checked_idx++] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if (temp == '\n')
        {
            if (m_checked_idx > 1 && m_read_buf[m_checked_idx - 1] == '\r')
            {
                m_read_buf[m_checked_idx - 1] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;//当前字节既不是\r，也不是\n,表示接收不完整，需要继续接收
}


//主状态机解析请求行，获得请求方法，目标url及http版本号，各个部分之间通过\t或空格分隔。
//strpbrk找到空格\t，strncasecmp匹配字符串，strspn跳过空格\t，
http_conn::HTTP_CODE http_conn::parse_request_line(char *text){
    LOG_DEBUG("parse_request_line: ");
    //请求方式
    //请求行中最先含有空格和\t 任一 字符的位置并返回
    m_url = strpbrk(text, " \t");
    if (!m_url)////如果没有空格或\t，则报文格式有误
    {
        return BAD_REQUEST;//HTTP请求报文有语法错误
    }
    *m_url++ = '\0';////将该位置改为\0，用于将前面数据取出
	//取出数据，并通过与GET和POST比较，以确定请求方式
    char *method = text;
    if (strcasecmp(method, "GET") == 0)//比较字符串，忽略大小写，到'\0'停止比较
        m_method = GET;
    else if (strcasecmp(method, "POST") == 0)
    {
        m_method = POST;
        cgi = 1;
    }
    else return BAD_REQUEST;
    m_url += strspn(m_url, " \t");//跳过直到不是' '或'\t'


    //版本,m_version指针进行处理
    m_version = strpbrk(m_url, " \t");
    if (!m_version)
        return BAD_REQUEST;
    *m_version++ = '\0';
    m_version += strspn(m_version, " \t");
    //本项目仅支持HTTP/1.1
    if (strcasecmp(m_version, "HTTP/1.1") != 0)
        return BAD_REQUEST;


    //资源链接
    //这里主要是有些报文的请求资源中会带有"http://"，这里需要对这种情况进行单独处理
    if (strncasecmp(m_url, "http://", 7) == 0)
    {
        m_url += 7;
        m_url = strchr(m_url, '/');//找第一次出现的位置
    }
	//同样增加https情况
    if (strncasecmp(m_url, "https://", 8) == 0)
    {
        m_url += 8;
        m_url = strchr(m_url, '/');
    }

    if(!m_url||m_url[0]!='/') return BAD_REQUEST;
    //当url为/时，显示判断界面
    if (strlen(m_url) == 1)
        strcat(m_url, "judge.html");//欢迎页面
    m_check_state = CHECK_STATE_HEADER;//请求行处理完毕，将主状态机转移处理请求头
    return NO_REQUEST;
}

//主状态机解析报文中的请求头数据
/*
    判断是空行还是请求头，若是空行，进而判断content-length是否为0，如果不是0，表明是POST请求，则状态转移到CHECK_STATE_CONTENT，否则说明是GET请求，则报文解析结束。
    若解析的是请求头部字段，则主要分析connection字段，content-length字段，其他字段可以直接跳过，各位也可以根据需求继续分析。
    connection字段判断是keep-alive还是close，决定是长连接还是短连接
    content-length字段，这里用于读取post请求的消息体长度
*/
http_conn::HTTP_CODE http_conn::parse_headers(char *text){
    LOG_DEBUG("parse_headers: ");
    //判断是空行还是请求头
    if (text[0] == '\0')
    {
        LOG_DEBUG("空行: text[0] == '\0': ");
		//判断是GET还是POST请求
        if (m_content_length != 0)
        {
			//POST需要跳转到消息体处理状态
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;//get无消息体，完成解析
    }
	 //解析请求头部Connection连接字段
    else if (strncasecmp(text, "Connection:", 11) == 0)
    {
        LOG_DEBUG("-->Connection: ");
        text += 11;
		//跳过空格和\t字符
        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0)//如果是长连接，则将linger标志设置为true
        {
            LOG_DEBUG("m_linger = true;");
            m_linger = true;
        }
    }
    else if (strncasecmp(text, "Content-length:", 15) == 0)//解析请求头部内容长度字段
    {
        LOG_DEBUG("-->Content-length:: ");
        text += 15;
        text += strspn(text, " \t");
        m_content_length = atol(text);
    }
	//解析请求头部HOST字段
    else if (strncasecmp(text, "Host:", 5) == 0)
    {
        LOG_DEBUG("-->Host: ");
        text += 5;
        text += strspn(text, " \t");
        m_host = text;
    }
    else
    {
        LOG_INFO("oop!unknow header: %s", text);
    }
    return NO_REQUEST;
}

//解析消息体（Post)
http_conn::HTTP_CODE http_conn::parse_content(char *text){
    if (m_read_idx >= (m_content_length + m_checked_idx))
    {
        text[m_content_length] = '\0'; //判断buffer中是否读取了消息体
        //POST请求中最后为输入的用户名和密码
        m_string = text;
        return GET_REQUEST;//获得了完整的HTTP请求
    }
    return NO_REQUEST;
}

//从m_read_buf读取，//有限状态机处理请求报文
http_conn::HTTP_CODE http_conn::process_read(){
    LOG_DEBUG("process_read: ");
    //初始化状态
    LINE_STATUS line_status = LINE_OK;//LINE_OK完整读取一行
    HTTP_CODE parse_ret = NO_REQUEST;//请求不完整
    char *text = 0;

    //循环：(若解析消息体（POST请求）&&从OK）||（从 解析了完整一行OK）
    while((m_check_state==CHECK_STATE_CONTENT&&line_status==LINE_OK)||((line_status = parse_line())==LINE_OK)){
        text = get_line();//将缓冲区指针指向主未解析的头部

        m_start_line = m_checked_idx;
        LOG_INFO("get_line: %s", text);

        //主状态转换
        switch(m_check_state){
            case CHECK_STATE_REQUESTLINE://请求行
            {
                LOG_DEBUG("case CHECK_STATE_REQUESTLINE: ");
                parse_ret = parse_request_line(text);
                if (parse_ret == BAD_REQUEST)//结果语法有错404
                    return BAD_REQUEST;
                break;
            }
            case CHECK_STATE_HEADER: //请求头
            {
                LOG_DEBUG("case CHECK_STATE_HEADER: ");
                parse_ret = parse_headers(text);
                if(parse_ret == BAD_REQUEST){
                    return BAD_REQUEST;
                }
                else if(parse_ret = GET_REQUEST){
                    LOG_DEBUG("parse_headers-->do_request");
                    return do_request();
                }
                break;
            }
            case CHECK_STATE_CONTENT://请求体（Post)
            {
                parse_ret = parse_content(text);
                //完整解析POST请求后，跳转到报文响应函数
                if (parse_ret == GET_REQUEST)
                    return do_request();

			    //解析完消息体即完成报文解析，避免再次进入循环，更新line_status
                line_status = LINE_OPEN;//读取的行不完整
                break;
            }
            default: return INTERNAL_ERROR;//服务器内部错误500
        }
    }
    return NO_REQUEST;
}

//主状态机解析完后，完成资源路径m_real_file拼接（通过stat判断该文件属性）,打开资源文件,完成文件地址指针到内存的映射
//定位最后一个'/'的位置p,判断p+1的数字，完成响应的(注册登陆)校验判断逻辑
http_conn::HTTP_CODE http_conn::do_request(){
    strcpy(m_real_file, doc_root);//将网站根目录和url文件拼接
    int len = strlen(doc_root);
    printf("m_url:%s\n", m_url);
	//m_url为请求行中解析出的(parse_request_line())请求资源
	//m_url中搜索最后一次出现字符'/'的位置，返回剩下的字符串
    const char *p = strrchr(m_url, '/');

    //处理cgi---POST请求标志
	//实现登录和注册校验---'2'为POST登录校验，'3'为POST注册校验
    if (cgi == 1 && (*(p + 1) == '2' || *(p + 1) == '3'))
    {
        //根据标志判断是登录检测还是注册检测
        //char flag = m_url[1];

        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/");
        strcat(m_url_real, m_url + 2);//+2：当前m_url_real为请求行/2或/3后的数据
        strncpy(m_real_file + len, m_url_real, FILENAME_LEN - len - 1);//FILENAME_LEN==200
        //此时m_real_file为doc_root+(m_url + 2)
		free(m_url_real);

        //将用户名和密码提取出来
        //user=123&passwd=123
        char name[100], password[100];
        int i;
		//m_string：请求体数据：包含用户名密码
        for (i = 5; m_string[i] != '&'; ++i)//前5个字符为"name=",'&'隔开密码
            name[i - 5] = m_string[i];
        name[i - 5] = '\0';

        int j = 0;
        for (i = i + 10; m_string[i] != '\0'; ++i, ++j)//前10个字符为"publisher="
            password[j] = m_string[i];
        password[j] = '\0';

        if (*(p + 1) == '3')//'3'为POST注册校验
        {
            //如果是注册，先检测数据库中是否有重名的
            //没有重名的，进行增加数据
            char *sql_insert = (char *)malloc(sizeof(char) * 200);
            strcpy(sql_insert, "INSERT INTO user(username, passwd) VALUES(");
            strcat(sql_insert, "'");
            strcat(sql_insert, name);
            strcat(sql_insert, "', '");
            strcat(sql_insert, password);
            strcat(sql_insert, "')");
			//"INSERT INTO user(username, passwd) VALUES('name', 'password')"
            
			//同步线程登录校验
			//users记录了doc用户和数据库用户的映射
			if (users.find(name) == users.end())
            {
                m_lock.lock();//访问数据库和映射表 同步线程
                int res = mysql_query(mysql, sql_insert);//访问数据库
                users.insert(pair<string, string>(name, password));//添加用户记录
                m_lock.unlock();

                if (!res)//==0
                    strcpy(m_url, "/log.html");//登录页
                else
                    strcpy(m_url, "/registerError.html");//注册错误
            }
            else
                strcpy(m_url, "/registerError.html");//注册用户已经存在
        }
        //如果是登录，直接判断
        //若浏览器端输入的用户名和密码在表中可以查找到，返回1，否则返回0
        else if (*(p + 1) == '2')
        {
            if (users.find(name) != users.end() && users[name] == password)
                strcpy(m_url, "/welcome.html");
            else
                strcpy(m_url, "/logError.html");
        }
    }

    if (*(p + 1) == '0')//POST,跳转到注册页面
    {
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/register.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    else if (*(p + 1) == '1')//跳转登录页面
    {
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/log.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    else if (*(p + 1) == '5')//图片请求页面
    {
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/picture.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    else if (*(p + 1) == '6')//视频请求页面
    {
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/video.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    else if (*(p + 1) == '7')//关注页面
    {
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/fans.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    else////如果以上均不符合，即不是登录和注册，直接将url与网站目录拼接
        strncpy(m_real_file + len, m_url, FILENAME_LEN - len - 1);
	
	//通过stat获取请求资源文件信息，成功则将信息更新到m_file_stat结构体
	//记录了文件类型权限大小字节数信息
    if (stat(m_real_file, &m_file_stat) < 0)//文件信息不存在
        return NO_RESOURCE;//404
	//判断文件的权限，是否可读，不可读则返回FORBIDDEN_REQUEST状态
    if (!(m_file_stat.st_mode & S_IROTH))
        return FORBIDDEN_REQUEST;//403
	//判断文件类型，如果是目录，则返回BAD_REQUEST，表示请求报文有误
    if (S_ISDIR(m_file_stat.st_mode))
        return BAD_REQUEST;//400
	//以只读方式获取文件描述符，通过mmap将该文件映射到内存中
    int fd = open(m_real_file, O_RDONLY);
    m_file_address = (char *)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    return FILE_REQUEST;//表示请求文件存在，且可以访问


}

//完成报文响应,将iovec指针指向报文写缓冲区和资源地址
bool http_conn::process_write(HTTP_CODE ret){//ret为precess_read（）或do_requset（）的返回值
    LOG_DEBUG("process_write");
    switch(ret){
        case NO_RESOURCE://404 文件信息不存在
        {
            add_status_line(404, error_404_title);//添加状态行：http/1.1 状态码 状态消息
            add_headers(strlen(error_404_form));//消息报头
            if (!add_content(error_404_form))//写入超出缓冲最大长度或空间不足
                return false;
        break;
        }
        case FORBIDDEN_REQUEST://403
        {
            add_status_line(403, error_403_title);
            add_headers(strlen(error_403_form));
            if (!add_content(error_403_form))
                return false;
        }
        case BAD_REQUEST://400
        {
            add_status_line(400, error_400_title);
            add_headers(strlen(error_400_form));
            if (!add_content(error_400_form))
                return false;
        }
        case INTERNAL_ERROR://500
        {
            add_status_line(500, error_500_title);
            add_headers(strlen(error_500_form));
            if (!add_content(error_500_form))
                return false;
        break;
        }
        case FILE_REQUEST://200
        {
            add_status_line(200, ok_200_title);
            //  判断是否还有资源
            if(m_file_stat.st_size>0){
                add_headers(m_file_stat.st_size);
                //第一个iovec指针指向响应报文缓冲区，长度指向m_write_idx
                m_iv[0].iov_base = m_write_buf;
                m_iv[0].iov_len = m_write_idx;
			    //第二个iovec指针指向mmap返回的文件指针，长度指向文件大小
                m_iv[1].iov_base = m_file_address;//
                m_iv[1].iov_len = m_file_stat.st_size;
                m_iv_count = 2;
			    //发送的全部数据为响应报文头部信息和文件大小
                bytes_to_send = m_write_idx + m_file_stat.st_size;
                //LOG_INFO("response:%s", m_write_buf);//一起写入日志
                return true;
            }
            else{
                //如果请求的资源大小为0，则返回空白html文件
                const char *ok_string = "<html><body>资源不足</body></html>";
                add_headers(strlen(ok_string));
                if (!add_content(ok_string))
                    //LOG_INFO("response:%s", m_write_buf);//一起写入日志
                    return false;
            }
        }
        default:return false;
    }
    //除FILE_REQUEST状态外，其余状态只申请一个iovec，指向响应报文缓冲区
    m_iv[0].iov_base = m_write_buf;
    m_iv[0].iov_len = m_write_idx;
    m_iv_count = 1;
    bytes_to_send = m_write_idx;
    //LOG_INFO("response:%s", m_write_buf);//一起写入日志
    return true;
}

void http_conn::unmap(){
    if (m_file_address)
    {
        munmap(m_file_address, m_file_stat.st_size);
        m_file_address = 0;//NULL
    }
}

//根据响应报文格式，生成对应8个部分，以下函数均由do_request调用
//add_response为基础函数，va_list存储可变长参数列表
bool http_conn::add_response(const char *format, ...){
    if (m_write_idx >= WRITE_BUFFER_SIZE)//如果需要写入大内容怎么办？
        return false;
    va_list arg_list;//解决变参问题的一组宏
    va_start(arg_list, format);//绑定
    //vsnprintf写入缓冲区，返回值需要>=0&&<WRITE_BUFFER_SIZE - 1 - m_write_idx
    int len = vsnprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - 1 - m_write_idx, format, arg_list);
    //如果写入的数据长度超过缓冲区剩余空间，则报错
	if (len >= (WRITE_BUFFER_SIZE - 1 - m_write_idx))
    {
        va_end(arg_list);
        return false;
    }
    //写入成功，更新m_write_idx
    m_write_idx+=len;
    va_end(arg_list);
    LOG_INFO("request:%s", m_write_buf);//在process中一并写入
    return true;
}   
bool http_conn::add_status_line(int status, const char *title){
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}
bool http_conn::add_headers(int content_length){
    return add_content_length(content_length) && add_linger() &&
           add_content_type()&&add_blank_line();
}
bool http_conn::add_content_type(){
    return add_response("Content-Type:%s\r\n", "text/html");
}
bool http_conn::add_content_length(int content_length){
    return add_response("Content-Length:%d\r\n", content_length);
}
bool http_conn::add_linger(){
    return add_response("Connection:%s\r\n", (m_linger == true) ? "keep-alive" : "close");
}
bool http_conn::add_blank_line(){
    return add_response("%s", "\r\n");
}
bool http_conn::add_content(const char *content){
    return add_response("%s", content);
}
