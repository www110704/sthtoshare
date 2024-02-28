#ifndef LOG_H
#define LOG_H

#include<stdarg.h>//_VA_ARGS_  可变参数宏
#include<stdio.h>
#include<string.h>
#include"blocking_queue.h"

//局部静态懒汉单列模式
class Log
{
    public:
    //单列模式，返回唯一实力
    static Log* get_instance(){
        static Log log;
        return &log;
    }
    //作为init创建线程时的回调函数（回调成员函数必须是static,不然参数会有this指针）
    static void* flush_log_thread(void* args){Log::get_instance()->async_write_log();}
    
    //初始化日志并（可选创建线程）写日志
    bool init(const char* file_name,int close_log,int log_buf_size = 8192,int max_lines = 5000000,int max_que_size = 0);
    //整理输出日志格式,level代表等级，0debug,1warn,2info,3error
    void write_log(int level,const char* format,...);
    //刷缓存，调用API fflush(FILE*)
    void flush(void);

    private:
    Log();
    virtual ~Log();
    //从阻塞队列中取出一个日志string，写入文件流
    void* async_write_log(){
        string str_log;
        //队列不停出栈，写入文件
        while(m_log_que->pop(str_log)){
            m_mutex.lock();
            fputs(str_log.c_str(),m_fp);//API将字符串写入FILE文件
            m_mutex.unlock();
        }
    }

    private://变量
    block_queue<string>* m_log_que;//循环阻塞队列
    FILE* m_fp; //文件指针
    char dir_name[128];//路径名
    char log_name[128];//log文件名
    int m_buf_size;//日志缓冲区大小
    char* m_buf;//缓冲区
    int m_max_lines;//最大行数，一条日志为一行
    int m_count;//当前行数
    int m_today;//记录日期
    bool m_is_async;//异步标志
    locker m_mutex;//队列,文件等锁
    int m_close_log_flag;//是否启用日志


};

//可变参数宏
//#define myfunc(format,...) func(format,##__VA_ARGS__)
//日志分级，debug,warn,info,error
//若日志开关打开，则写各种日志并强制刷新缓冲区
#define LOG_DEBUG(format,...) if(0==m_close_log_flag){Log::get_instance()->write_log(0,format,##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_INFO(format,...) if(0==m_close_log_flag){Log::get_instance()->write_log(1,format,##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_WARN(format,...) if(0==m_close_log_flag){Log::get_instance()->write_log(2,format,##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_ERROR(format,...) if(0==m_close_log_flag){Log::get_instance()->write_log(3,format,##__VA_ARGS__); Log::get_instance()->flush();}

#endif // !LOG_H