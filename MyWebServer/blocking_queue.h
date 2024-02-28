//循环数组实现的阻塞队列，m_back = (m_back + 1) % m_max_size;  

#ifndef BLOCKING_QUEUE_H
#define BLOCKING_QUEUE_H

#include<iostream>
#include<stdlib.h>
#include<pthread.h>
#include<sys/time.h>
#include"locker.h"


//T = string类型
template<class T>
class block_queue
{
    public:
    block_queue(int max_size = 1000){
        if(max_size<=0) exit(-1);
        m_max_size = max_size;
        m_array = new T[max_size];
        m_size = 0;
        m_front = -1;
        m_back = -1;
    }
    ~block_queue(){//删除array
        m_mutex.lock();
        if(m_array!=nullptr) delete[] m_array;
        m_mutex.unlock();
    }
    void clear()
    {
        m_mutex.lock();
        m_size = 0;
        m_front = -1;
        m_back = -1;
        m_mutex.unlock();
    }
    bool full() 
    {
        m_mutex.lock();
        if (m_size >= m_max_size)
        {

            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }
    bool empty() 
    {
        m_mutex.lock();
        if (0 == m_size)
        {
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }
    //返回队首元素
    bool front(T &value) 
    {
        m_mutex.lock();
        if (0 == m_size)
        {
            m_mutex.unlock();
            return false;
        }
        value = m_array[m_front];
        m_mutex.unlock();
        return true;
    }
    //返回队尾元素
    bool back(T &value) 
    {
        m_mutex.lock();
        if (0 == m_size)
        {
            m_mutex.unlock();
            return false;
        }
        value = m_array[m_back];
        m_mutex.unlock();
        return true;
    }
    int size() 
    {
        int tmp = 0;

        m_mutex.lock();
        tmp = m_size;

        m_mutex.unlock();
        return tmp;
    }
    int max_size()
    {
        int tmp = 0;

        m_mutex.lock();
        tmp = m_max_size;

        m_mutex.unlock();
        return tmp;
    }

    //往队列添加元素，需要将所有使用队列的线程先唤醒
    //当有元素push进队列,相当于生产者生产了一个元素
    //若当前没有线程等待条件变量,则唤醒无意义
    bool push(const T &item){
        m_mutex.lock();
        //判断队列满了？
        if(m_size>=m_max_size){
            m_cond.broadcast();//唤醒线程处理
            m_mutex.unlock();
            return false;
        }
        m_back = (m_back+1)%m_max_size;//循环覆盖
        m_array[m_back] = item;
        m_size++;
        m_cond.broadcast();

        m_mutex.unlock();
        return true;
    }

    //阻塞等待获取元素
    bool pop(T& item){
        m_mutex.lock();
        while(m_size<=0){//while--多个消费者都在等没所以需要一直等到轮到自己为止
            if(!m_cond.wait(m_mutex.get())){//内部有一次解索和加锁
                m_mutex.unlock();
                return false;
            }
        }
        m_front = (m_front + 1) % m_max_size;//循环覆盖
        item = m_array[m_front];
        m_size--;
        m_mutex.unlock();
        return true;
    }

    //增加了超时处理
	//在pthread_cond_wait基础上增加了等待的时间，只指定时间内能抢到互斥锁即可
    //其他逻辑不变
    bool pop(T &item, int ms_timeout)
    {
        //？？？
        struct timespec t = {0, 0};
        struct timeval now = {0, 0};
        gettimeofday(&now, NULL);
        m_mutex.lock();
        if (m_size <= 0)
        {
            t.tv_sec = now.tv_sec + ms_timeout / 1000;
            t.tv_nsec = (ms_timeout % 1000) * 1000;
            if (!m_cond.timewait(m_mutex.get(), t))//在规定时间t内争夺锁
            {
                m_mutex.unlock();
                return false;
            }
        }
        //抢到了锁但是没有资源，直接失败?
        if (m_size <= 0)
        {
            m_mutex.unlock();
            return false;
        }

        m_front = (m_front + 1) % m_max_size;
        item = m_array[m_front];
        m_size--;
        m_mutex.unlock();
        return true;
    }

    private:
    locker m_mutex;
    cond m_cond;//阻塞队列的信号量，异步写日志时阻塞和唤醒io线程

    T *m_array;//循环数组，循环覆盖
    int m_size;//资源量
    int m_max_size;//队列长度
    int m_front;//循环数组的对头
    int m_back;//循环数组的队尾
};



#endif
