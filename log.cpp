/**
 * Created by wonder on 2020/10/9.
 **/

#include "log.h"

namespace wonder {

Log::Log() {
    m_lineCount = 0;
    m_outPath = nullptr;
    m_suffix = nullptr;
    m_level = LOG_LEVEL::INFO;
    m_today = 0;
    m_buf = nullptr;
    m_queue = nullptr;
    m_writeThread = nullptr;
    m_fp = nullptr;
}
Log::~Log() {
    if (m_writeThread && m_writeThread->joinable()) {
        while (!m_queue->empty()) {
            m_queue->flush();
        }
        m_queue->stop();
        m_writeThread->join();
    }
    if (m_fp) {
        std::lock_guard<std::mutex> locker(m_mutex);
        flush();
        fclose(m_fp);
    }
}
void Log::init(LOG_LEVEL level, const char *path, const char *suffix, int maxQueueSize) {

    if (!m_queue) {
        std::unique_ptr<BlockQueue<std::string>> newBlockQueue(new BlockQueue<std::string>(maxQueueSize));
        m_queue = std::move(newBlockQueue);

        std::unique_ptr<std::thread> newThread(new std::thread(FlushLogThread));
        m_writeThread = std::move(newThread);
    }
    //初始化日志器等级
    m_level = level;
    //初始化行号
    m_lineCount = 0;
    //初始化日志输出地
    m_outPath = path;
    //初始化日志后缀
    m_suffix = suffix;
    //初始化写日志缓冲区
    m_buf = new char[LOG_BUF_LEN];
    memset(m_buf, '\0', LOG_BUF_LEN);
    //得到当前系统时间
    time_t timer = time(nullptr);
    struct tm *sysTime = localtime(&timer);
    //保存日志文件创建的时间
    m_today = sysTime->tm_mday;
    //初始化日志文件
    char fileName[LOG_NAME_LEN] = {0};
    snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s",
             m_outPath, sysTime->tm_year + 1900, sysTime->tm_mon + 1, sysTime->tm_mday, m_suffix);

    //保证互斥的操作文件
    {
        std::lock_guard<std::mutex> locker(m_mutex);

        //若文件已经被打开，则刷新缓冲区，再关闭文件
        if (m_fp) {
            flush();
            fclose(m_fp);
        }
        //以追加的模式打开文件
        m_fp = fopen(fileName, "a");
        //若打开文件失败，则尝试以当前用户先创建文件夹
        if (m_fp == nullptr) {
            mkdir(m_outPath, 0777);
            m_fp = fopen(fileName, "a");
        }
        assert(m_fp != nullptr);
    }
}
void Log::write(LOG_LEVEL level, const char *format, ...) {
    //得到当前时间
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t tSec = now.tv_sec;
    struct tm *sysTime = localtime(&tSec);
    va_list vaList;

    // 若当前时间已经和创建文件时的时间不匹配了
    // 或当前日志行数已经大于一个文件能容纳的行数了
    // 则重新创建文件
    if (m_today != sysTime->tm_mday || (m_lineCount && (m_lineCount % LOG_MAX_LINES == 0))) {
        char newFileName[LOG_NAME_LEN];
        char tail[32] = {0};
        //格式化日志文件的中间部分
        snprintf(tail, 32, "%04d_%02d_%02d", sysTime->tm_year + 1900,
                 sysTime->tm_mon + 1, sysTime->tm_mday);
        //若当前时间与创建日志文件时间不匹配，则创建新的日志文件
        if (m_today != sysTime->tm_mday) {
            snprintf(newFileName, LOG_NAME_LEN, "%s/%s%s", m_outPath, tail, m_suffix);
            m_today = sysTime->tm_mday;
            m_lineCount = 0;
        } else {  //若是因为当前日志文件已经写满，则创建新的日志文件,在原来的文件名的基础上加上后缀
            snprintf(newFileName, LOG_NAME_LEN, "%s/%s-%d%s", m_outPath, tail,
                     m_lineCount / LOG_MAX_LINES, m_suffix);
        }

        std::unique_lock<std::mutex> locker(m_mutex); //离开作用域后自动析构
        locker.lock();
        flush();
        fclose(m_fp);

        m_fp = fopen(newFileName, "a");
        assert(m_fp != nullptr);
    }
    {
        std::unique_lock<std::mutex> locker(m_mutex);
        //当前行数加1
        m_lineCount++;
        //写入具体的时间格式
        int n = snprintf(m_buf, LOG_BUF_LEN, "%s %d-%02d-%02d %02d:%02d:%02d.%06ld ",
                         getLevelTitle(level), sysTime->tm_year + 1900, sysTime->tm_mon + 1,
                         sysTime->tm_mday, sysTime->tm_hour, sysTime->tm_min, sysTime->tm_sec, now.tv_usec);
        //处理可变参数
        va_start(vaList, format);
        int m = vsnprintf(m_buf + n, LOG_BUF_LEN - 1, format, vaList);
        va_end(vaList);
        m_buf[n + m] = '\n';
        m_buf[n + m + 1] = '\0';

        //放入阻塞队列中
        if (m_queue && !m_queue->full()) {
            m_queue->push(m_buf);
        }
    }
}
//得到当前level对应的字符串指针
const char *Log::getLevelTitle(wonder::LOG_LEVEL level) {
    std::string str;
    switch (level) {
        case LOG_LEVEL::INFO: str = "[INFO]:";
            break;
        case LOG_LEVEL::DEBUG:str = "[DEBUG]:";
            break;
        case LOG_LEVEL::WARN:str = "[WARN]:";
            break;
        case LOG_LEVEL::ERROR:str = "[ERROR]:";
            break;
        default:str = "[INFO]:";
    }
    return str.c_str();
}
LOG_LEVEL Log::getLevel() {
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_level;
}
void Log::flush() {
    m_queue->flush();
    fflush(m_fp);
}
//从阻塞队列中读取日志，写入文件
void Log::asyncWrite() {
    std::string str = "";
    while (m_queue->get(str)) {
        std::lock_guard<std::mutex> locker(m_mutex);
        fputs(str.c_str(), m_fp);
    }
}
//得到日志器的实例
Log *Log::Instance() {
    static Log instance;
    return &instance;
}
//刷新写日志线程
void Log::FlushLogThread() {
    Log::Instance()->asyncWrite();
}

}   //namespace