#ifndef THREADPOOL_H
#define THREADPOOL_H

#include<list>
#include<cstdio>
#include<exception>
#include<pthread.h>

#include"locker.h"
#include"sql_connection_pool.h"

using namespace std;

//模板类的定义实现需要在同一文件内
//T为http_conn类型
template <typename T>
class threadpool
{
public:
    threadpool(int actor_model,connection_pool* connPool,int thread_num = 8,int max_request = 10000);
    ~threadpool();
    bool append(T* request,int state);
    bool append_p(T* request);

private:
    //工作线程运行函数
    static void* worker(void* arg);//调用run
    void run();

private:
    int m_thread_num;   //线程数量
    int m_max_request;  //请求队列中最大请求数量
    pthread_t* m_threads;//线程池数组，存放递增id
    list<T*> m_workqueue;//请求队列
    locker m_queLocker; //队列互斥索
    sem m_questat;      //是否有任务要处理
    connection_pool* m_connPool;//连接池
    int m_actor_model;//模式切换,1为Reactor模式,其他为Preactor
    
};



//创建线程数组作为id,创建线程池,调用worker(this)作为回调函数,并调用run()开始执行
template <typename T>
inline threadpool<T>::threadpool(int actor_model, connection_pool *connPool, int thread_num, int max_request)
:m_actor_model(actor_model),m_thread_num(thread_num),m_max_request(max_request),m_threads(nullptr),m_connPool(connPool)
{
    if(thread_num<=0||max_request<=0) throw exception();
    m_threads = new pthread_t[thread_num];//线程数组,创建线程的参数
    if(!m_threads) throw exception();
    //创建线程
    for (int  i = 0; i < thread_num; i++)
    {
        if(pthread_create(m_threads+i,NULL,worker,this)!=0){//手动分配线程id
            delete[] m_threads;
            throw exception();
        }
        if(pthread_detach(m_threads[i])){//分离线程,执行完自动回收
            delete[] m_threads;
            throw exception();
        }
    }
}

template <typename T>
inline threadpool<T>::~threadpool()
{
    delete[] m_threads;
}

//将 T 任务添加到队列----Reactor模式,交给工作线程处理读写任务,需要判断state 杜还是写
template <typename T>
inline bool threadpool<T>::append(T *request, int state)
{
    m_queLocker.lock();
    //若队列超出最大值,返回false
    if(m_workqueue.size()>=m_max_request){
        m_queLocker.unlock();
        return false;
    }
    request->m_state = state;//0为读,1为写任务
    m_workqueue.push_back(request);
    m_queLocker.unlock();
    m_questat.post();//V操作+1唤醒阻塞中的工作线程
    return true;
}

//将 T 任务添加到队列---Preactor模式,主线程直接处理IO,所以不需要判断state
template <typename T>
inline bool threadpool<T>::append_p(T *request)
{
    m_queLocker.lock();
    if(m_workqueue.size()>=m_max_request){
        m_queLocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queLocker.unlock();
    m_questat.post();//V
    return false;
}

//创建thread时自动调用,参数为this
template <typename T>
inline void *threadpool<T>::worker(void *arg)
{
    threadpool* pool  = (threadpool*)arg;//传入this指针
    pool->run();
    return pool;//为什么返回this?
}


//工作线程
template <typename T>
inline void threadpool<T>::run()
{
    while(true){
        m_questat.wait();//P操作-1阻塞
        m_queLocker.lock();//加锁取出任务
        if(m_workqueue.empty()){
            m_queLocker.unlock();
            continue;
        }
        T* request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queLocker.unlock();

        if(!request) continue;//任务不为空
        if(m_actor_model==1){//Reactor,根据state判断读写
            if(request->m_state==0){//read
                if(request->read_once()){
                    request->improv = 1;//完成了读
                    connRAII sqlconn(&request->mysql,m_connPool);//连接数据库
                    request->process();
                }
                else{
                    request->improv = 1;
                    request->timer_flag = 1;//表示关闭连接?
                }
            }
            else{//write
                if(request->write()){
                    request->improv = 1;
                }
                else{
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
        }
        else{//Preactor,主线程eventLoop IO完了,子线程处理逻辑proess
            connRAII sqlconn(&request->mysql,m_connPool);
            request->process();
        }
    }
}

#endif