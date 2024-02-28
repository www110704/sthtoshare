/*****************************************************************************
主要函数
read_once()         从用户fd读取数据到读缓冲区
process()           处理读缓冲区中的数据,解析报文,相应报文

process_read()      状态机解析报文,返回解析情况HTTP_CODE
do_request()        根据请求完成资源路径拼接,获取文件信息stat,打开资源文件,完成文件地址指针到内存的映射
process_write()     完成报文响应,将iovec指针指向报文写缓冲区和资源地址

write()             从写缓冲区发送数据到浏览器端，并重新初始化http对象
*******************************************************************************/

#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include<unistd.h>//提供对POSIX可移植操作系统借口API的访问能力

#include<sys/epoll.h>
#include<sys/types.h>//基本系统数据类型
#include<fcntl.h>//常用文件读写操作
#include<sys/socket.h>
#include<sys/stat.h>//获取文件属性
#include<sys/mman.h>//内存映射
#include<sys/wait.h>//进程等待
#include<sys/uio.h>//定义了struct iovc io向量

#include <signal.h>//信号处理部分， 定义了程序执行时如何处理不同的信号
#include <netinet/in.h>//定义了socketaddr_in结构体
#include <arpa/inet.h>//提供ip地址转换函数
#include <assert.h>
#include <string.h>//strcpy ,strcmp ,strcat等函数
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>//va_list头文件
#include <errno.h>
#include <map>

#include"locker.h"
#include"log.h"
#include"sql_connection_pool.h"
#include"timer_lst.h"

class http_conn
{
public://预设变量
    static const int FILENAME_LEN = 200;        //读取文件的名称m_real_file大小
    static const int READ_BUFFER_SIZE = 2048;   //读缓冲区m_read_buf大小
    static const int WRITE_BUFFER_SIZE = 1024;  //写缓冲区m_write_buf大小
    //报文请求方式
    enum METHOD{GET = 0,POST,HEAD,PUT,DELETE,TRACE,OPTIONS,CONNECT,PATH};
    //报文解析的结果
    enum HTTP_CODE{NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION};
    //主状态机器状态
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    //从状机器状态
    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

public:
    http_conn() {}
    ~http_conn() {}

    //初始化mysql连接，获取sql中所有用户名和密码并存入全局map中
    void initmysql_result(connection_pool *connPool);
    //初始化套接字地址，函数内部会调用私有方法init
    void init(int sockfd, const sockaddr_in &addr, char *root, int TRIGMode,int close_log, string user, string passwd, string sqlname);
    void process();
    bool read_once();
    bool write();
    void close_conn(bool real_close = true);
    sockaddr_in *get_address()
    {
        return &m_address;
    }
    
private:
    void init();
	//从m_read_buf读取，并处理请求报文
    HTTP_CODE process_read();
	//完成报文响应,将iovec指针指向报文写缓冲区和资源地址
    bool process_write(HTTP_CODE ret);
	//主状态机解析报文中的请求行数据
    HTTP_CODE parse_request_line(char *text);
	//主状态机解析报文中的请求头数据
    HTTP_CODE parse_headers(char *text);
	//主状态机解析报文中的请求内容(仅用于POST请求)
    HTTP_CODE parse_content(char *text);
	//根据请求完成资源路径拼接,打开资源文件,完成文件地址指针到内存的映射
    HTTP_CODE do_request();
    
    //get_line用于将指针向后偏移，指向未处理的字符
    char *get_line() { return m_read_buf + m_start_line; };//m_start_line是已经解析的字符

    //从状态机读取一行，分析是请求报文的哪一部分
	LINE_STATUS parse_line();
	
    void unmap();

    //根据响应报文格式，生成对应8个部分，以下函数均由do_request调用
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();


//变量
public:
    static int m_epollfd;//epoll根结点
    static int m_user_count;//http用户连接的数量
    MYSQL *mysql;//mysql服务区指针
	
    int m_state;  //线程池append参数,读为0, 写为1

    int timer_flag;//定时器标志
    int improv;//?已更新？？

private:
    //socket相关
    int m_sockfd;
    sockaddr_in m_address;

    //buf相关
    char m_read_buf[READ_BUFFER_SIZE];//存储读取的请求报文数据
    int m_read_idx;//缓冲区中 从 解析完数据的最后一个字节的下一个位置
    int m_checked_idx;//从 解析的位置
    int m_start_line;//主 已经解析的字符个数

    char m_write_buf[WRITE_BUFFER_SIZE];//存储发出的响应报文数据
    int m_write_idx;//指示buffer中的长度

    //enum相关
    CHECK_STATE m_check_state;//主状态机的状态
    METHOD m_method;//请求方法

    //解析请求报文相关
    char m_real_file[FILENAME_LEN];//资源文件的完整路径
    char *m_url;
    char *m_version;
    char *m_host;
    int m_content_length;
    bool m_linger;//连接状态："keep-alive" or "close"

    //响应报文相关
    char *m_file_address;//服务器上的资源文件地址
    struct stat m_file_stat;//存储文件状态信息
    struct iovec m_iv[2]; //io向量机制iovec，指向写缓冲区和资源文件
    int m_iv_count;//m_iv大小
    int cgi;        //是否启用的POST
    char *m_string; //post消息体
    int bytes_to_send;//剩余发送字节数
    int bytes_have_send;//已发送字节数
    char *doc_root;//doc根用户名
    map<string, string> m_users;//记录doc用户和数据库用户的映射
    int m_TRIGMode;//触发模式
    int m_close_log_flag;//关闭日志


    //数据库相关
    char sql_user[100];//数据库用户名
    char sql_passwd[100];//数据库密码
    char sql_name[100];//数据库名称

};


#endif // !HTTP_CONN_H