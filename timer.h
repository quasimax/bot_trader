#ifndef TIMER_H
#define TIMER_H

#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <boost/function.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include <iostream>
#include <map>

namespace asio = boost::asio;


class Timer : public std::enable_shared_from_this<Timer>, private asio::deadline_timer
{
public:
    Timer(asio::io_context& ioc, boost::function<void ()> func) : asio::deadline_timer(ioc), m_func(func)
    {

    }

    void start(int interval, bool periodic)
    {
        m_interval = interval;
        m_periodic = periodic;
        run();
    }

    void inline stop() {cancel();}

private:
    int m_interval{0};
    int m_periodic{false};
    boost::function<void ()> m_func;

private:
    void run()
    {
        expires_from_now(boost::posix_time::seconds(m_interval));
        async_wait(boost::bind(&Timer::timeout,
                                       this,
                                       boost::asio::placeholders::error));
    }

    void timeout(const boost::system::error_code& ec)
    {
        if(ec)
            return;

        m_func();

        if(m_periodic)
            run();
    }
};

class TimerMap
{
public:
    TimerMap(asio::io_context& ioc) : m_ioc(ioc) {}
    ~TimerMap()
    {
        while(!m_timers.empty())
        {
            auto it = m_timers.begin();
            it->second->stop();
            m_timers.erase(it);
        }
    }

    void add_timer(const std::string& name, boost::function<void ()> func)
    {
        boost::shared_ptr<Timer> dtp = boost::make_shared<Timer>(m_ioc, func);
        m_timers.emplace(name, dtp);
    }

    void del_timer(const std::string& name)
    {
        auto it = m_timers.find(name);

        if(m_timers.end() == it)
            return;

        it->second->stop();
        m_timers.erase(it);
    }

    void start_timer(const std::string& name, int interval, bool periodic)
    {
        auto it = m_timers.find(name);

        if(m_timers.end() == it)
            return;

        it->second->start(interval, periodic);
    }

    void stop_timer(const std::string& name)
    {
        auto it = m_timers.find(name);

        if(m_timers.end() == it)
            return;

        it->second->stop();
    }

private:
    asio::io_context& m_ioc;
    std::map<const std::string, boost::shared_ptr<Timer>> m_timers;
};


#endif // TIMER_H
