## 日志系统

LogLevel：日志级别  
LogEvent：日志事件，日志输出的内容信息等  
LogEventWrap：管理日志事件，RAII机制  
LogFormatter：日志格式器  
LogAppender：日志输出地  
Logger：日志输出器  

### 类图
![log](./log.png)

### 执行流程
logger初始化：new LogFormatter->init()：解析模板，回调函数返回的对象指针保存在m_items  
logger->log(level, event)：遍历成员appenders，appender->log(logger, level, event)->调用formatter的format->遍历m_items，指针调用format返回正确的日志流