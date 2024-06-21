/*
 * @Author       : mark
 * @Date         : 2020-06-15
 * @copyleft Apache 2.0
 */ 

#ifndef ThreadPool_H
#define ThreadPool_H

#include <assert.h>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>

class cThreadPool {
public:
    explicit cThreadPool(size_t ThreadCount = 8): m_Pool(std::make_shared<sPool>()) {
            assert(ThreadCount > 0);
            for(size_t i = 0; i < ThreadCount; i++) {
                std::thread([pool = m_Pool] {
                    std::unique_lock<std::mutex> locker(pool->m_mtx);
                    while(true) {
                        if(!pool->m_tasks.empty()) {
                            auto task = std::move(pool->m_tasks.front());
                            pool->m_tasks.pop();
                            locker.unlock();
                            task();
                            locker.lock();
                        } 
                        else if(pool->m_IsClosed) break;
                        else pool->m_cond.wait(locker);
                    }
                }).detach();
            }
    }

    cThreadPool() = default;

    cThreadPool(cThreadPool&&) = default;
    
    ~cThreadPool() {
        if(static_cast<bool>(m_Pool)) {
            {
                std::lock_guard<std::mutex> locker(m_Pool->m_mtx);
                m_Pool->m_IsClosed = true;
            }
            m_Pool->m_cond.notify_all();
        }
    }

    template<class F>
    void AddTask(F&& a_Task) {
        {
            std::lock_guard<std::mutex> locker(m_Pool->m_mtx);
            m_Pool->m_tasks.emplace(std::forward<F>(a_Task));
        }
        m_Pool->m_cond.notify_one();
    }

private:
    struct sPool {
        std::mutex m_mtx;
        std::condition_variable m_cond;
        bool m_IsClosed;
        std::queue<std::function<void()>> m_tasks;
    };
    std::shared_ptr<sPool> m_Pool;
};


#endif //cThreadPool_H