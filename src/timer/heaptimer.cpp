/*
 * @Author       : mark
 * @Date         : 2020-06-17
 * @copyleft Apache 2.0
 */ 
#include "heaptimer.h"

void cHeapTimer::siftup(size_t i) {
    assert(i >= 0 && i < m_heap.size());
    size_t j = (i - 1) / 2;
    while(j >= 0) {
        if(m_heap[j] < m_heap[i]) { break; }
        SwapNode(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}

void cHeapTimer::SwapNode(size_t i, size_t j) {
    assert(i >= 0 && i < m_heap.size());
    assert(j >= 0 && j < m_heap.size());
    std::swap(m_heap[i], m_heap[j]);
    m_ref[m_heap[i].m_id] = i;
    m_ref[m_heap[j].m_id] = j;
} 

bool cHeapTimer::siftdown(size_t a_index, size_t n) {
    assert(a_index >= 0 && a_index < m_heap.size());
    assert(n >= 0 && n <= m_heap.size());
    size_t i = a_index;
    size_t j = i * 2 + 1;
    while(j < n) {
        if(j + 1 < n && m_heap[j + 1] < m_heap[j]) j++;
        if(m_heap[i] < m_heap[j]) break;
        SwapNode(i, j);
        i = j;
        j = i * 2 + 1;
    }
    return i > a_index;
}

void cHeapTimer::add(int a_id, int a_timeout, const TimeoutCallBack& a_cb) {
    assert(a_id >= 0);
    size_t i;
    if(m_ref.count(a_id) == 0) {
        /* 新节点：堆尾插入，调整堆 */
        i = m_heap.size();
        m_ref[a_id] = i;
        m_heap.push_back({a_id, Clock::now() + MS(a_timeout), a_cb});
        siftup(i);
    } 
    else {
        /* 已有结点：调整堆 */
        i = m_ref[a_id];
        m_heap[i].m_Expires = Clock::now() + MS(a_timeout);
        m_heap[i].m_cb = a_cb;
        if(!siftdown(i, m_heap.size())) {
            siftup(i);
        }
    }
}

void cHeapTimer::doWork(int a_id) {
    /* 删除指定id结点，并触发回调函数 */
    if(m_heap.empty() || m_ref.count(a_id) == 0) {
        return;
    }
    size_t i = m_ref[a_id];
    sTimerNode node = m_heap[i];
    node.m_cb();
    del(i);
}

void cHeapTimer::del(size_t a_index) {
    /* 删除指定位置的结点 */
    assert(!m_heap.empty() && a_index >= 0 && a_index< m_heap.size());
    /* 将要删除的结点换到队尾，然后调整堆 */
    size_t i = a_index;
    size_t n = m_heap.size() - 1;
    assert(i <= n);
    if(i < n) {
        SwapNode(i, n);
        if(!siftdown(i, n)) {
            siftup(i);
        }
    }
    /* 队尾元素删除 */
    m_ref.erase(m_heap.back().m_id);
    m_heap.pop_back();
}

void cHeapTimer::adjust(int a_id, int a_timeout) {
    /* 调整指定id的结点 */
    assert(!m_heap.empty() && m_ref.count(a_id) > 0);
    m_heap[m_ref[a_id]].m_Expires = Clock::now() + MS(a_timeout);
    siftdown(m_ref[a_id], m_heap.size());
}

void cHeapTimer::tick() {
    /* 清除超时结点 */
    if(m_heap.empty()) {
        return;
    }
    while(!m_heap.empty()) {
        sTimerNode node = m_heap.front();
        if(std::chrono::duration_cast<MS>(node.m_Expires - Clock::now()).count() > 0) { 
            break; 
        }
        node.m_cb();
        pop();
    }
}

void cHeapTimer::pop() {
    assert(!m_heap.empty());
    del(0);
}

void cHeapTimer::clear() {
    m_ref.clear();
    m_heap.clear();
}

int cHeapTimer::GetNextTick() {
    tick();
    size_t res = -1;
    if(!m_heap.empty()) {
        res = std::chrono::duration_cast<MS>(m_heap.front().m_Expires - Clock::now()).count();
        if(res < 0) { res = 0; }
    }
    return res;
}