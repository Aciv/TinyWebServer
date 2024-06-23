/*
 * @Author       : mark
 * @Date         : 2020-06-16
 * @copyleft Apache 2.0
 */ 
#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include "../log/log.h"

class cSqlConnPool {
public:
    static cSqlConnPool *Instance();

    MYSQL *GetConn();
    void FreeConn(MYSQL * a_conn);
    int GetFreeConnCount();

    void Init(const char* a_host, int a_port,
              const char* a_user,const char* a_pwd, 
              const char* a_dbName, int a_connSize);
    void ClosePool();

private:
    cSqlConnPool();
    ~cSqlConnPool();

    int m_MaxConn;
    int m_UseCount;
    int m_FreeCount;

    std::queue<MYSQL *> m_ConnQue;
    std::mutex m_mtx;
    sem_t m_semId;
};


#endif // SQLCONNPOOL_H