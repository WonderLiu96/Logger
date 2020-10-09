# Logger
一个C11实现的简单日志器,使用阻塞队列异步进行写日志的操作。
# 使用
```
#include "log.h"
//初始化日志器
wonder::Log::Instance()->init();
//支持自定义格式
LOG_INFO(fmt,...)
```
# 输出
[INFO]: 2020-10-09 14:26:35.788051 This is info.

[WARN]: 2020-10-09 14:26:35.788070 WARN