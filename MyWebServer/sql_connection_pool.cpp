#include"sql_connection_pool.h"

connection_pool::connection_pool(){
    m_FreeConn = 0;
    m_CurConn = 0;
}

connection_pool::~connection_pool(){
    DestroyPool();
}

//当有请求时从数据库中返回一个可用连接，更新使用和空闲连接数
MYSQL *connection_pool::GetConn()
{   
    MYSQL* conn = nullptr;
    if(connList.size()==0) return nullptr;
    reserve.wait();//P操作-1阻塞

    lock.lock();
        conn = connList.front();
        connList.pop_front();
        --m_FreeConn;
        ++m_CurConn;
    lock.unlock();

    return conn;
}

bool connection_pool::ReleaseConn(MYSQL *conn)
{
    if(conn==nullptr) return false;
    lock.lock();
    connList.push_back(conn);
    ++m_FreeConn;
    --m_CurConn;
    lock.unlock();
    
    reserve.post();//V操作+1唤醒
    return true;
}

void connection_pool::DestroyPool()
{
    lock.lock();
    if(connList.size()>0){
        list<MYSQL*>::iterator it;
        for ( it = connList.begin(); it != connList.end(); ++it)
        {
            MYSQL* con = *it;
            mysql_close(con);//关闭连接
        }
        m_CurConn = 0;
        m_FreeConn = 0;
        connList.clear();
    }
    lock.unlock();
}

//局部静态懒汉单列模式，c++11开始支持
connection_pool *connection_pool::GetInstance()
{
    static connection_pool connPool;//c++11内部支持局部静态的原子性
    return &connPool;
}


void connection_pool::init(string url, string User, string Password, string DatabaseName, int port, int MaxConn, int close_log)
{
    m_url = url;
    m_user = User;
    m_password = Password;
    m_databaseName = DatabaseName;
    m_close_log_flag = close_log;

    for (int i = 0; i < MaxConn; i++)//创建MaxConn个连接诶
    {
        MYSQL*con = nullptr;
        //获取并连接MYSQL服务器
        con = mysql_init(con);
        if(con==nullptr){
            LOG_ERROR("MYSQL init Error");//LOG_ERROR为定义的可变参数宏,在log.h中定义
            exit(1);
        }
        //连接数据库引擎
        con = mysql_real_connect(con,url.c_str(),User.c_str(),Password.c_str(),DatabaseName.c_str(),port,NULL,0);
        if(!con){
            LOG_ERROR("MYSQL real connect Error");
            exit(1);
        }
        connList.push_back(con);//加入连接池
        ++m_FreeConn;//空闲连接+1
    }
    reserve = sem(m_FreeConn);//信号量为连接池连接总数
    m_MaxConn = m_FreeConn;
}


//构造函数,需要改变指针的指向
//每一条连接RAII,创建及获取连接池的一条连接,用完及释放连接
connRAII::connRAII(MYSQL **con, connection_pool *connPool)
{
    *con = connPool->GetConn();
    connectionRAII = *con;
    poolRAII = connPool;
}

connRAII::~connRAII()
{
    poolRAII->ReleaseConn(connectionRAII);
}
