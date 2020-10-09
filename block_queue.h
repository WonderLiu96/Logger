/**
 * Created by wonder on 2020/10/9.
 **/

#ifndef LOGGER__BLOCK_QUEUE_H_
#define LOGGER__BLOCK_QUEUE_H_

#include <condition_variable>
#include <list>
template<typename T>
class BlockQueue {
public:
    explicit BlockQueue(int maxSize)
        : m_maxSize(maxSize),
          m_needStop(false) {
    }
    explicit BlockQueue()
        : m_maxSize(1024),
          m_needStop(false) {
    }
    void push(const T &x) {
        add(x);
    }
    void push(T &&x) {
        add(std::forward<T>(x));
    }
    bool get(T &t) {
        std::unique_lock<std::mutex> locker(m_mutex);
        m_notEmpty.wait(locker, [this] { return m_needStop || notEmpty(); });
        if (m_needStop) {
            return false;
        }
        t = m_queue.front();
        m_queue.pop_front();
        m_notFull.notify_one();
        return true;
    }
    void stop() {
        {
            std::lock_guard<std::mutex> locker(m_mutex);
            m_needStop = true;
        }
        m_notFull.notify_all();
        m_notEmpty.notify_all();
    }
    bool empty() {
        std::lock_guard<std::mutex> locker(m_mutex);
        return m_queue.empty();
    }
    bool full() {
        std::lock_guard<std::mutex> locker(m_mutex);
        return m_queue.size() == m_maxSize;
    }
    size_t size() {
        std::lock_guard<std::mutex> locker(m_mutex);
        return m_queue.size();
    }
    void flush() {
        m_notEmpty.notify_one();
    }
private:
    bool notEmpty() const {
        bool empty = m_queue.empty();
        if (empty) {
//            std::cout << "缓冲区为空,需要等待..." << std::endl;
        }
        return !empty;
    }
    bool notFull() const {
        bool full = m_queue.size() >= m_maxSize;
        if (full) {
//            std::cout << "缓冲区已满,需要等待..." << std::endl;
        }
        return !full;
    }
    template<typename F>
    void add(F &&x) {
        std::unique_lock<std::mutex> locker(m_mutex);
        m_notFull.wait(locker, [this] { return m_needStop || notFull(); });
        if (m_needStop) {
            return;
        }
        m_queue.push_back(std::forward<F>(x));
        m_notEmpty.notify_one();
    }
private:
    std::list<T> m_queue;                   //缓冲区
    std::mutex m_mutex;                     //互斥变量
    std::condition_variable m_notEmpty;     //不为空的条件变量
    std::condition_variable m_notFull;      //没有满的条件变量
    int m_maxSize;                          //阻塞队列最大的size
    bool m_needStop;                        //停止标志
};

#endif //LOGGER__BLOCK_QUEUE_H_
