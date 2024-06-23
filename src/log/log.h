/*
 * @Author       : mark
 * @Date         : 2020-06-16
 * @copyleft Apache 2.0
 */ 
#ifndef LOG_H
#define LOG_H

#include <mutex>
#include <string>
#include <thread>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>           // vastart va_end
#include <assert.h>
#include <sys/stat.h>         //mkdir
#include "blockqueue.h"
#include "../buffer/buffer.h"

class cLog {
public:
    void init(int level, const char* path = "./log", 
                const char* suffix =".log",
                int maxQueueCapacity = 1024);

    static cLog* Instance();
    static void FlushLogThread();

    void write(int a_level, const char *a_format,...);
    void flush();

    int GetLevel();
    void SetLevel(int a_level);
    bool IsOpen() { return m_IsOpen; }
    
private:
    cLog();
    void AppendLogLevelTitle(int a_level);
    virtual ~cLog();
    void AsyncWrite();

private:
    static const int LOG_PATH_LEN = 256;
    static const int LOG_NAME_LEN = 256;
    static const int MAX_LINES = 50000;

    const char* m_path;
    const char* m_suffix;

    int m_LineCount;
    int m_ToDay;

    bool m_IsOpen;
 
    cBuffer m_buff;
    int m_level;
    bool m_IsAsync;

    FILE* m_fp;
    std::unique_ptr<BlockDeque<std::string>> m_deque; 
    std::unique_ptr<std::thread> m_writeThread;
    std::mutex m_mtx;
};

#define LOG_BASE(a_level, a_format, ...) \
    do {\
        cLog* log = cLog::Instance();\
        if (log->IsOpen() && log->GetLevel() <= a_level) {\
            log->write(a_level, a_format, ##__VA_ARGS__); \
            log->flush();\
        }\
    } while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);

#endif //LOG_H