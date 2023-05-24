#include "logger.h"

Logger m_logger;
Logger s_logger;

void mlogger::init()
{
    log4cplus::initialize();

     log4cplus::BasicConfigurator config;
     config.configure();
}

void mlogger::init_common_log(const std::string& log_file)
{
     SharedAppenderPtr rolling_main_App(new RollingFileAppender(log_file, 10 *1024 *1024, 3));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

     std::string  pattern =  "%d{%m-%d-%y %H:%M:%S} %m%n";
     std::auto_ptr<Layout> layout(new  PatternLayout(pattern));
     rolling_main_App->setLayout(layout);

#pragma GCC diagnostic pop

     m_logger = Logger::getInstance("main");
     m_logger.addAppender(rolling_main_App);
}

void mlogger::init_script_log(const std::string& log_file)
{
    SharedAppenderPtr rolling_script_App(new RollingFileAppender(log_file));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

    std::string  pattern =  "%d{%m-%d-%y %H:%M:%S}  %m%n";
    std::auto_ptr<Layout> layout(new  PatternLayout(pattern));
    rolling_script_App->setLayout(layout);

#pragma GCC diagnostic pop

    s_logger = Logger::getInstance("script");
    s_logger.addAppender(rolling_script_App);

}

Logger* mlogger::m_p_logger(){return & m_logger;}
Logger* mlogger::s_p_logger(){return & s_logger;}

void mlogger::deinit()
{
    Logger::shutdown();
}
