#include "log.h"

namespace sylar {

Logger::Logger(const std::string& name)
    : m_name(name)
{

}

void Logger::addAppender(LogAppender::ptr appender)
{

}

void Logger::delAppender(LogAppender::ptr appender)
{
    for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it)
    {
        if (*it == appender)
        {
            m_appenders.erase(it);
            break;
        }
    }
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event)
{
    if (level >= m_level)
    {
        for (auto& i : m_appenders)
        {
            i->log(level, event);
        }
    }
}
    
void Logger::debug(LogEvent::ptr event)
{
    log(LogLevel::Level::DEBUG, event);
}

void Logger::info(LogEvent::ptr event)
{
    log(LogLevel::Level::INFO, event);
}

void Logger::warn(LogEvent::ptr event)
{
    log(LogLevel::Level::WARN, event);
}

void Logger::fatal(LogEvent::ptr event)
{
    log(LogLevel::Level::FATAL, event);
}

void Logger::error(LogEvent::ptr event)
{
    log(LogLevel::Level::ERROR, event);
}

void StdoutLogAppender::log(LogLevel::Level level, LogEvent::ptr event)
{
    if (level >= m_level)
    {
        std::cout << m_formatter->format(event);
    }
}

FileLogAppender::FileLogAppender(const std::string& filename)
    : m_filename(filename)
{

}

void FileLogAppender::log(LogLevel::Level level, LogEvent::ptr event)
{
    if (level >= m_level)
    {
        m_filestream << m_formatter->format(event);
    }
}

bool FileLogAppender::reopen()
{
    if (m_filestream)
    {
        m_filestream.close();
    }
    m_filestream.open(m_filename);
    return !!m_filestream;
}

LogFormatter::LogFormatter(const std::string& pattern)
    : m_pattern(pattern)
{

}

void LogFormatter::init()
{
    // str, format, type
    std::vector<std::tuple<std::string, std::string, int>> vec;
    std::string str;
    for (size_t i = 0; i < m_pattern.size(); ++i)
    {
        if (m_pattern[i] != '%')
        {
            str.append(1, m_pattern[i]);
            continue;
        }
        size_t n = i+1;
        int fmt_status = 0;

        std::string str;
        std::string fmt;
        while (n < m_pattern.size())
        {
            if (isspace(m_pattern[n]))
                break;
            if (fmt_status == 0)
            {
                if (m_pattern[n] == '{')
                {
                    str = m_pattern.substr(i + 1, n - i);
                    fmt_status = 1; // 解析格式
                    continue;
                }
            }
        }
    }
}

std::string LogFormatter::format(LogEvent::ptr event)
{
    std::stringstream ss;
    for (auto& i : m_items)
    {
        i->format(ss, event);
    }
    return ss.str();
}

}