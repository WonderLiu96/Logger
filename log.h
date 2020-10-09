/**
 * Created by wonder on 2020/10/9.
 **/

#ifndef LOGGER__LOG_H_
#define LOGGER__LOG_H_

#include <iostream>
#include <cstdio>
#include <cstring>
#include <thread>
#include <mutex>
#include <cassert>
#include <sys/stat.h>
#include <sys/time.h>
#include <cstdarg>
#include "block_queue.h"
namespace wonder {

enum LOG_LEVEL {
    INFO, DEBUG, WARN, ERROR
};
class Log {
public:
    void init(LOG_LEVEL level = LOG_LEVEL::INFO, const char *path = "./log", const char *suffix = ".log",
              int maxQueueSize = 1024);

    static Log *Instance();
    static void FlushLogThread();

    void write(LOG_LEVEL level, const char *format, ...);
    void flush();

    LOG_LEVEL getLevel();
    void setLevel(LOG_LEVEL level);

/// debug function
    bool queueIsEmpty() {
        return m_queue->empty();
    }

private:
    Log();
    virtual ~Log();
    Log(const Log &);
    Log &operator=(const Log &);

    const char *getLevelTitle(LOG_LEVEL level);
    void asyncWrite();
private:
    //日志缓冲区最大长度
    static const int LOG_BUF_LEN = 1024;
    //日志名最大长度
    static const int LOG_NAME_LEN = 256;
    //一个日志文件所能容纳的最大行数
    static const int LOG_MAX_LINES = 50000;

    //当前日志的行数
    int m_lineCount;
    //日志的输出位置
    const char *m_outPath;
    //日志的后缀名
    const char *m_suffix;
    //当前日志的等级
    LOG_LEVEL m_level;
    //创建日志文件的时间(为了保证同一天的日志同一个文件保存)
    int m_today;
    //当前日志文件的描述符
    FILE *m_fp;
    //写入日志缓冲区
    char *m_buf;
    //异步队列
    std::unique_ptr<BlockQueue<std::string>> m_queue;
    //写日志的线程
    std::unique_ptr<std::thread> m_writeThread;
    //日志器的互斥量
    std::mutex m_mutex;
};

#define LOG_BASE(level, format, ...) \
do{ \
    wonder::Log * log = wonder::Log::Instance(); \
    if(log->getLevel() <= level) { \
        log->write(level,format,##__VA_ARGS__); \
        log->flush(); \
    } \
}while(0);

#define LOG_INFO(format, ...) do{ LOG_BASE(wonder::LOG_LEVEL::INFO,format,##__VA_ARGS__);}while(0);
#define LOG_DEBUG(format, ...) do{ LOG_BASE(wonder::LOG_LEVEL::DEBUG,format,##__VA_ARGS__);}while(0);
#define LOG_WARN(format, ...) do{ LOG_BASE(wonder::LOG_LEVEL::WARN,format,##__VA_ARGS__);}while(0);
#define LOG_ERROR(format, ...) do{ LOG_BASE(wonder::LOG_LEVEL::ERROR,format,##__VA_ARGS__);}while(0);

#endif //LOGGER__LOG_H_

} //namespace
