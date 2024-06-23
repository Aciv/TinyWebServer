/*
 * @Author       : mark
 * @Date         : 2020-06-17
 * @copyleft Apache 2.0
 */ 
#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h> 
#include <functional> 
#include <assert.h> 
#include <chrono>
#include "../log/log.h"

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

struct sTimerNode {
    int m_id;
    TimeStamp m_Expires;
    TimeoutCallBack m_cb;
    bool operator<(const sTimerNode& t) {
        return m_Expires < t.m_Expires;
    }
};
class cHeapTimer {
public:
    cHeapTimer() { m_heap.reserve(64); }

    ~cHeapTimer() { clear(); }
    
    void adjust(int a_id, int a_newExpires);

    void add(int a_id, int a_TimeOut, const TimeoutCallBack& a_cb);

    void doWork(int a_id);

    void clear();

    void tick();

    void pop();

    int GetNextTick();

private:
    void del(size_t i);
    
    void siftup(size_t i);

    bool siftdown(size_t a_index, size_t n);

    void SwapNode(size_t i, size_t j);

    std::vector<sTimerNode> m_heap;

    std::unordered_map<int, size_t> m_ref;
};

#endif //HEAP_TIMER_H