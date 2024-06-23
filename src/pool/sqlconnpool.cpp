/*
 * @Author       : mark
 * @Date         : 2020-06-17
 * @copyleft Apache 2.0
 */ 

#include "sqlconnpool.h"
using namespace std;

cSqlConnPool::cSqlConnPool() {
    m_UseCount = 0;
    m_FreeCount = 0;
}

cSqlConnPool* cSqlConnPool::Instance() {
    static cSqlConnPool connPool;
    return &connPool;
}

void cSqlConnPool::Init(const char* a_host, int a_port,
            const char* a_user,const char* a_pwd, const char* a_dbName,
            int a_connSize = 10) {
    assert(a_connSize > 0);
    for (int i = 0; i < a_connSize; i++) {
        MYSQL *sql = nullptr;
        sql = mysql_init(sql);
        if (!sql) {
            LOG_ERROR("MySql init error!");
            assert(sql);
        }
        sql = mysql_real_connect(sql, a_host,
                                 a_user, a_pwd,
                                 a_dbName, a_port, nullptr, 0);
        if (!sql) {
            LOG_ERROR("MySql Connect error!");
        }
        m_ConnQue.push(sql);
    }
    m_MaxConn = a_connSize;
    sem_init(&m_semId, 0, m_MaxConn);

}

MYSQL* cSqlConnPool::GetConn() {
    MYSQL *sql = nullptr;
    if(m_ConnQue.empty()){
        LOG_WARN("SqlConnPool busy!");
        return nullptr;
    }
    sem_wait(&m_semId);
    {
        lock_guard<mutex> locker(m_mtx);
        sql = m_ConnQue.front();
        m_ConnQue.pop();
    }
    return sql;
}

void cSqlConnPool::FreeConn(MYSQL* a_sql) {
    assert(a_sql);
    lock_guard<mutex> locker(m_mtx);
    m_ConnQue.push(a_sql);
    sem_post(&m_semId);
}

void cSqlConnPool::ClosePool() {
    lock_guard<mutex> locker(m_mtx);
    while(!m_ConnQue.empty()) {
        auto item = m_ConnQue.front();
        m_ConnQue.pop();
        mysql_close(item);
    }
    mysql_library_end();        
}

int cSqlConnPool::GetFreeConnCount() {
    lock_guard<mutex> locker(m_mtx);
    return m_ConnQue.size();
}

cSqlConnPool::~cSqlConnPool() {
    ClosePool();
}
