#ifndef LOGGER_H
#define LOGGER_H

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/configurator.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/fileappender.h>
#include <log4cplus/appender.h>

#include <string>
#include <memory>

using namespace log4cplus;

class mlogger
{
    static Logger* s_p_logger();
    static Logger* m_p_logger();

public:
    static void init();
    static void init_common_log(const std::string& log_file);
    static void init_script_log(const std::string& log_file);
    static void deinit();

    template<typename... Args>
    static void info(const char* fmt, Args&&... args)
    {
        LOG4CPLUS_INFO_FMT(*m_p_logger(), fmt, std::forward<Args>(args)...);
    }

    static void info(const char* msg)
    {
        LOG4CPLUS_INFO(*m_p_logger(), msg);
    }

    template<typename... Args>
    static void warn(const char* fmt, Args&&... args)
    {
        LOG4CPLUS_WARN_FMT(*m_p_logger(), fmt, std::forward<Args>(args)...);
    }

    static void warn(const char* msg)
    {
        LOG4CPLUS_WARN(*m_p_logger(), msg);
    }

    template<typename... Args>
    static void error(const char* fmt, Args&&... args)
    {
        LOG4CPLUS_ERROR_FMT(*m_p_logger(), fmt, std::forward<Args>(args)...);
    }

    static void error(const char* msg)
    {
        LOG4CPLUS_ERROR(*m_p_logger(), msg);
    }

    template<typename... Args>
    static void __info(const char* fmt, Args&&... args)
    {
        LOG4CPLUS_INFO_FMT(*s_p_logger(), fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void __warn(const char* fmt, Args&&... args)
    {
        LOG4CPLUS_WARN_FMT(*s_p_logger(), fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void __error(const char* fmt, Args&&... args)
    {
        LOG4CPLUS_ERROR_FMT(*s_p_logger(), fmt, std::forward<Args>(args)...);
    }
};

#endif // LOGGER_H
