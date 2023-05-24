#ifndef STRATEGYSERVER_H
#define STRATEGYSERVER_H

#include <memory>
#include <map>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/move/unique_ptr.hpp>
#include <boost/function.hpp>
#include <boost/asio/deadline_timer.hpp>

#include <boost/python.hpp>
#include <boost/python/wrapper.hpp>

#include "bnc_data_types.h"
#include "tkeyboard.h"
#include "timer.h"

namespace python = boost::python;
namespace asio = boost::asio;



typedef struct
{
    boost::function<void (const std::string&, bool)> on_send_message;
    boost::function<void (const std::string&)> on_request_value;
    boost::function<void (const tlg_keyboard_t&)> on_request_command;
    boost::function<void (const bnc_order_t&, OrderAction)> on_create_order;
    boost::function<void (const bnc_order_t&)> on_delete_order;
    boost::function<void ()> on_delete_all_orders;
    boost::function<void (const std::string&)> on_get_price;
    boost::function<void (const std::string&)> on_add_stream;
    boost::function<void (const std::string&)> on_del_stream;
    boost::function<void (const std::string&)> on_trade_state;
    boost::function<void (const std::string&)> on_journal_data;
    boost::function<void (const std::string&)> on_alert_msg;

    boost::function<void (const std::string&, boost::function<void ()>)> on_create_timer;
    boost::function<void (const std::string&, int, bool)> on_start_timer;
    boost::function<void (const std::string&)> on_stop_timer;
    boost::function<void (const std::string&)> on_del_timer;

} ss_callbacks_t;

class StrategyServer;

class StrategyInstance
{
public:
    StrategyInstance(StrategyServer&);
    virtual ~StrategyInstance() = default;

    virtual void eval() = 0;

    void execOrder(bnc_order_t& order);
    void deleteOrder(const bnc_order_t& order);
    void deleteAllOrders();    
    void sendMessage(const std::string msg, bool alw);
    void requestValue(const std::string msg);
    void requestCommand(const std::string& msg, const python::list& list, TlgKeyboard::Side orientation);
    void toLog(const std::string& msg);
    void getPrice(const std::string& symbol);
    void addStream(const std::string& stream);
    void delStream(const std::string& stream);
    void toFront(const std::string& data);
    void toJournal(const std::string& data);
    void toAlertChannel(const std::string& data);

    void createTimer(const std::string& name, python::object function);
    void startTimer(const std::string& name, boost::uint64_t interval, bool recurring = false);
    void stopTimer(const std::string& name);
    void delTimer(const std::string& name);

    std::string format_error(python::handle<> const&, python::handle<> const&, python::handle<> const&);

    virtual void onSymbolsList(python::list& symbols_list) = 0;
    virtual void onOrderReport(const std::string&) = 0;
    virtual void onAlert(const std::string& name, const std::string& body) = 0;
    virtual void getIndicators() = 0;
    virtual void onPrice(const std::string& data) = 0;
    virtual void onWssData(const std::string& data) = 0;
    virtual void onAccount(const std::string&) = 0;
    virtual void onBalance(const std::string&) = 0;
    virtual void onUserValue(const std::string&) = 0;
    virtual void onTradeData(const std::string&) = 0;
    virtual void onGetTradeState(const std::string&) = 0;
    virtual python::list getTradeSteps(const std::string& data) = 0;
    virtual void onTradeRun(const std::string& data) = 0;
    virtual void onExchangeInfo(const std::string& data) = 0;
    virtual void getExchangeInfo() = 0;

    const ss_callbacks_t& callbacks();    

private:
    StrategyServer& m_server;
    ss_callbacks_t m_callbacks;
    python::object m_format_exception;
};

class StrategyServer
{
public:
    inline void set_callbacks(ss_callbacks_t& callbacks)
    {
        m_callbacks = callbacks;
    }

    void execOrder(StrategyInstance& instance, bnc_order_t& order);    
    void deleteOrder(StrategyInstance& instance, const bnc_order_t& order);
    void deleteAllOrders(StrategyInstance& instance);
    void sendMessage(StrategyInstance& instance, const std::string& msg, bool alw);
    void requestValue(StrategyInstance& instance, const std::string& msg);
    void requestCommand(StrategyInstance& instance, const std::string& msg, const python::list& list, TlgKeyboard::Side orientation);
    void toLog(StrategyInstance& instance, const std::string& msg);
    void getPrice(StrategyInstance& instance, const std::string& symbol);
    void addStream(StrategyInstance& instance, const std::string& stream);
    void delStream(StrategyInstance& instance, const std::string& stream);
    void tradeState(StrategyInstance& instance, const std::string& data);
    void toJournal(StrategyInstance& instance, const std::string& data);
    void toAlertChannel(StrategyInstance& instance, const std::string& msg);

    //void createTimer(StrategyInstance& instance, const std::string& name, python::object function, boost::uint64_t interval, bool recurring = false);
    //void stopTimer(StrategyInstance& instance, const std::string& name);

    void createTimer(StrategyInstance& instance, const std::string& name, python::object function);
    void startTimer(StrategyInstance& instance, const std::string& name, boost::uint64_t interval, bool recurring = false);
    void stopTimer(StrategyInstance& instance, const std::string& name);
    void delTimer(StrategyInstance& instance, const std::string& name);

    std::string format_error(StrategyInstance& instance, python::handle<> const&, python::handle<> const&, python::handle<> const&);

    const ss_callbacks_t& callbacks(){return m_callbacks;}

private:
    int _next_order_id = 0;        
    ss_callbacks_t m_callbacks;    
};


#endif // STRATEGYSERVER_H
