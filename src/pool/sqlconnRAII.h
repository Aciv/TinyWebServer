/*
 * @Author       : mark
 * @Date         : 2020-06-19
 * @copyleft Apache 2.0
 */ 

#ifndef SQLCONNRAII_H
#define SQLCONNRAII_H
#include "sqlconnpool.h"

/* 资源在对象构造初始化 资源在对象析构时释放*/
class cSqlConnRAII {
public:
    cSqlConnRAII(MYSQL** a_sql, cSqlConnPool *a_connpool) {
        assert(a_connpool);
        *a_sql = a_connpool->GetConn();
        m_sql = *a_sql;
        m_ConnPool = a_connpool;
    }
    
    ~cSqlConnRAII() {
        if(m_sql) { m_ConnPool->FreeConn(m_sql); }
    }
    
private:
    MYSQL *m_sql;
    cSqlConnPool* m_ConnPool;
};

#endif //SQLCONNRAII_H