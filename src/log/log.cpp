/*
 * @Author       : mark
 * @Date         : 2020-06-16
 * @copyleft Apache 2.0
 */ 
#include "log.h"

using namespace std;

cLog::cLog() {
    m_LineCount = 0;
    m_IsAsync = false;
    m_writeThread = nullptr;
    m_deque = nullptr;
    m_ToDay = 0;
    m_fp = nullptr;
}

cLog::~cLog() {
    if(m_writeThread && m_writeThread->joinable()) {
        while(!m_deque->empty()) {
            m_deque->flush();
        };
        m_deque->Close();
        m_writeThread->join();
    }
    if(m_fp) {
        lock_guard<mutex> locker(m_mtx);
        flush();
        fclose(m_fp);
    }
}

int cLog::GetLevel() {
    lock_guard<mutex> locker(m_mtx);
    return m_level;
}

void cLog::SetLevel(int a_level) {
    lock_guard<mutex> locker(m_mtx);
    m_level = a_level;
}

void cLog::init(int a_level = 1, const char* a_path, const char* a_suffix,
    int a_maxQueueSize) {
    m_IsOpen = true;
    m_level = a_level;
    if(a_maxQueueSize > 0) {
        m_IsAsync = true;
        if(!m_deque) {
            unique_ptr<BlockDeque<std::string>> newDeque(new BlockDeque<std::string>);
            m_deque = move(newDeque);
            
            std::unique_ptr<std::thread> NewThread(new thread(FlushLogThread));
            m_writeThread = move(NewThread);
        }
    } else {
        m_IsAsync = false;
    }

    m_LineCount = 0;

    time_t timer = time(nullptr);
    struct tm *sysTime = localtime(&timer);
    struct tm t = *sysTime;
    m_path = a_path;
    m_suffix = a_suffix;
    char fileName[LOG_NAME_LEN] = {0};
    snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s", 
            m_path, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, m_suffix);
    m_ToDay = t.tm_mday;

    {
        lock_guard<mutex> locker(m_mtx);
        m_buff.RetrieveAll();
        if(m_fp) { 
            flush();
            fclose(m_fp); 
        }

        m_fp = fopen(fileName, "a");
        if(m_fp == nullptr) {
            mkdir(m_path, 0777);
            m_fp= fopen(fileName, "a");
        } 
        assert(m_fp != nullptr);
    }
}

void cLog::write(int a_level, const char *a_format, ...) {
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t tSec = now.tv_sec;
    struct tm *sysTime = localtime(&tSec);
    struct tm t = *sysTime;
    va_list vaList;

    /* 日志日期 日志行数 */
    if (m_ToDay != t.tm_mday || (m_LineCount && (m_LineCount  %  MAX_LINES == 0)))
    {
        unique_lock<mutex> locker(m_mtx);
        locker.unlock();
        
        char newFile[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

        if (m_ToDay != t.tm_mday)
        {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", m_path, tail, m_suffix);
            m_ToDay = t.tm_mday;
            m_LineCount = 0;
        }
        else {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", m_path, tail, (m_LineCount  / MAX_LINES), m_suffix);
        }
        
        locker.lock();
        flush();
        fclose(m_fp);
        m_fp = fopen(newFile, "a");
        assert(m_fp != nullptr);
    }

    {
        unique_lock<mutex> locker(m_mtx);
        m_LineCount++;
        int n = snprintf(m_buff.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                    t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
                    
        m_buff.HasWritten(n);
        AppendLogLevelTitle(a_level);

        va_start(vaList, a_format);
        int m = vsnprintf(m_buff.BeginWrite(), m_buff.WritableBytes(), a_format, vaList);
        va_end(vaList);

        m_buff.HasWritten(m);
        m_buff.Append("\n\0", 2);

        if(m_IsAsync && m_deque && !m_deque->full()) {
            m_deque->push_back(m_buff.RetrieveAllToStr());
        } else {
            fputs(m_buff.Peek(), m_fp);
        }
        m_buff.RetrieveAll();
    }
}

void cLog::AppendLogLevelTitle(int a_level) {
    switch(a_level) {
    case 0:
        m_buff.Append("[debug]: ", 9);
        break;
    case 1:
        m_buff.Append("[info] : ", 9);
        break;
    case 2:
        m_buff.Append("[warn] : ", 9);
        break;
    case 3:
        m_buff.Append("[error]: ", 9);
        break;
    default:
        m_buff.Append("[info] : ", 9);
        break;
    }
}

void cLog::flush() {
    if(m_IsAsync) { 
        m_deque->flush(); 
    }
    fflush(m_fp);
}

void cLog::AsyncWrite() {
    string str = "";
    while(m_deque->pop(str)) {
        lock_guard<mutex> locker(m_mtx);
        fputs(str.c_str(), m_fp);
    }
}

cLog* cLog::Instance() {
    static cLog inst;
    return &inst;
}

void cLog::FlushLogThread() {
    cLog::Instance()->AsyncWrite();
}