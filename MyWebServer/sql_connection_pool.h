#ifndef SQL_CONNECTION_POOL
#define SQL_CONNECTION_POOL

#include<stdio.h>
#include<list>
#include<mysql/mysql.h>
#include<error.h>
#include<string>
#include<iostream>

#include "locker.h"
#include "log.h"

using namespace std;

//单列模式，仅有一个对象实力，利用RAII机制封装
class connection_pool
{
public:
    MYSQL* GetConn();               //获取一条连接
    bool ReleaseConn(MYSQL* conn);  //释放一条连接

    int GetFreeConn(){return this->m_FreeConn;}   //当前空闲连接数
    void DestroyPool();             //销毁所有连接

    static connection_pool* GetInstance();//单列模式

    void init(string url,string User,string Password,string DatabaseName,int port,int MaxConn,int close_log);

private:
    connection_pool();
    ~connection_pool();

    int m_MaxConn;//最大连接数
    int m_CurConn;//当前已经使用的连接数
    int m_FreeConn;//当前空闲的连接数
    locker lock;
    list<MYSQL*> connList;//连接池
    sem reserve;//预留储备

public:
    string m_url,m_port,m_user,m_password,m_databaseName;
    int m_close_log_flag;//日志开关
};


//封装连接池连连接，实现每一条连接的RAII机制
class connRAII{
    public:
    connRAII(MYSQL** con,connection_pool* connPool);//二层指针，修改传入的指针的指向
    ~connRAII();

    private:
    MYSQL* connectionRAII;
    connection_pool* poolRAII;
};


#endif // SQL_CONNECTION_POOL
