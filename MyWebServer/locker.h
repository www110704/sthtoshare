#ifndef LOCKER_H
#define LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>
using namespace std;

//封装线程共享信号量，实现RAII机制，创建及初始化，用完自动销毁
//用于数据库连接池和线程池，两者作用逻辑相反
//数据库sem初始化为最大连接数，获取连接时wait -1直到阻塞，释放连接时post +1唤醒
//线程池sem默认初始化0，创建线程时调用wait阻塞,加入任务队列调用post唤醒
class sem
{
public:
	sem(){
		if(sem_init(&m_sem,0,0)!=0){//Posix信号量操作，参数2==0，被进程内线程共享		
			throw std::exception();
		}
	}
	sem(int num){//设置信号量的初始值
		if(sem_init(&m_sem,0,num)!=0){//Posix信号量操作，参数2==0，被进程内线程共享		
			throw std::exception();
		}
	}
	~sem(){
		sem_destroy(&m_sem);
	}

	bool wait(){return sem_wait(&m_sem)==0;}//P操作
	bool post(){return sem_post(&m_sem)==0;}//V操作

private:
	sem_t m_sem;
};

//封装互斥锁，实现RAII机制
class locker{
	public:
	locker(){
		if(pthread_mutex_init(&m_mutex,NULL)!=0){
			throw std::exception();
		}
	}
	~locker(){
		pthread_mutex_destroy(&m_mutex);
	}

	bool lock() {return pthread_mutex_lock(&m_mutex);}
	bool unlock() {return pthread_mutex_unlock(&m_mutex);}
	pthread_mutex_t* get(){return & m_mutex;}
	private:
	pthread_mutex_t m_mutex;
};


//封装条件变量，实现RAII机制
//用于日志系统生产者-消费者模式，创建阻塞队列，实现阻塞和唤醒
class cond
{
	public:
	cond(){
		if(pthread_cond_init(&m_cond,NULL)!=0){
			throw std::exception();
		}
	}
	~cond(){
		pthread_cond_destroy(&m_cond);
	}

	//需要传入互斥锁，加入系统请求队列，函数内部先解索，被唤醒时再枷锁
	bool wait(pthread_mutex_t *m_mutex){
		int ret = 0;
		ret = pthread_cond_wait(&m_cond,m_mutex);
		return ret == 0;
	}

	//增加超时处理，在pthread_cond_wait基础上增加等待时间，在等待时间内抢到锁即可
	bool timewait(pthread_mutex_t* m_mutex,struct timespec t){
		int ret = 0;
		ret = pthread_cond_timedwait(&m_cond,m_mutex,&t);
		return ret==0;
	}
	//bool signal(){return pthread_cond_signal(&m_cond)==0;}//
	//广播唤醒所有阻塞的线程
	bool broadcast(){return pthread_cond_broadcast(&m_cond)==0;}

	private:
	pthread_cond_t m_cond;
};


#endif


