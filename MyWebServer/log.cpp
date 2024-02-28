#include"log.h"

using namespace std;


Log::Log(){
    m_count = 0;
    m_is_async = false;
}

Log::~Log(){//关闭文件指针
    if(m_fp){
        fclose(m_fp);
    }
}
//异步需要设置阻塞队列的长度(max_que_size)，同步不需要设置
//传入Log后缀，生成完成文件名， 并打开log文件
bool Log::init(const char *file_name, int close_log, int log_buf_size, int split_lines, int max_queue_size){
    //根据传入的队列长度判断是同步还是异步
    if(max_queue_size>=1){
        m_is_async = true;
        m_log_que = new block_queue<string>(max_queue_size);
        //创建线程,自动分配id
        pthread_t tid;
        pthread_create(&tid,NULL,flush_log_thread,NULL);//回调函数调用阻塞队列的出队阻塞函数pop()
    }

    //正常设置
    m_close_log_flag = close_log;
    m_buf_size = log_buf_size;
    m_buf = new char[log_buf_size];
    memset(m_buf, '\0', log_buf_size);//string.h头文件
    m_max_lines = split_lines;

    //获取时间time_t,struct tm
    time_t t = time(NULL);
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    //找到传入文件名最后一个‘/’符号，将后续的字符传给p指针
    const char* p = strrchr(file_name,'/');
    char log_full_name[256] = {0};//完整的文件名，按照当前时间来格式化

    if (p == NULL)//传入的filename是一个目录，则根据日期新建log_full_name
    {
        snprintf(log_full_name, 255, "%d_%02d_%02d_%s", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
    }
    else//传入的文件名是一个文件，则保存log后缀和文件路径，并拼接成完整的文件名
    {
        strcpy(log_name, p + 1);
        strncpy(dir_name, file_name, p - file_name + 1);
        snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, log_name);
    }
    m_today = my_tm.tm_mday;
    
    m_fp = fopen(log_full_name, "a");//追加
    if (m_fp == NULL)
    {
        return false;
    }

    return true;

}

////整理输出日志格式,level代表等级，0debug,1warn,2info,3error
void Log::write_log(int level,const char* format,...){
    //获取时间
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    time_t t = now.tv_sec;
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    char s[16] = {0};//Log前缀，表示每条日志的等级
    switch (level)
    {
    case 0:
        strcpy(s, "[debug]:");
        break;
    case 1:
        strcpy(s, "[info]:");
        break;
    case 2:
        strcpy(s, "[warn]:");
        break;
    case 3:
        strcpy(s, "[erro]:");
        break;
    default:
        strcpy(s, "[info]:");
        break;
    }
    //写入一个log，对m_count++, m_split_lines最大行数
    m_mutex.lock();
    m_count++;

    if (m_today != my_tm.tm_mday || m_count % m_max_lines == 0) //everyday log
    {
        
        char new_log[256] = {0};
        fflush(m_fp);
        fclose(m_fp);
        char tail[16] = {0};
       
        snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);
       
        if (m_today != my_tm.tm_mday)
        {
            snprintf(new_log, 255, "%s%s%s", dir_name, tail, log_name);
            m_today = my_tm.tm_mday;
            m_count = 0;
        }
        else
        {
            snprintf(new_log, 255, "%s%s%s.%d", dir_name, tail, log_name, m_count / m_max_lines);
        }
        m_fp = fopen(new_log, "a");
    }
    m_mutex.unlock();

    va_list valst;//存储边长参数，便于格式化输出
    va_start(valst, format);

    string log_str;
    m_mutex.lock();

    //写入的具体时间内容格式
    int n = snprintf(m_buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                     my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                     my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);
    
    int m = vsnprintf(m_buf + n, m_buf_size - 1, format, valst);
    m_buf[n + m] = '\n';
    m_buf[n + m + 1] = '\0';//终止符
    log_str = m_buf;//拷贝后解索，给其他线程使用

    m_mutex.unlock();

    
    if (m_is_async && !m_log_que->full())//异步push交给异步线程
    {
        m_log_que->push(log_str);
    }
    else//同步直接fputs写入
    {
        m_mutex.lock();
        fputs(log_str.c_str(), m_fp);
        m_mutex.unlock();
    }

    va_end(valst);

}

//刷文件缓存
void Log::flush(void){
    m_mutex.lock();
    fflush(m_fp);
    m_mutex.unlock();
}