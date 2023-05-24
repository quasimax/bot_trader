#include <iostream>

#include "strategyserver.h"
#include "logger.h"


StrategyInstance::StrategyInstance(StrategyServer& server) : m_server(server)
{    
    m_callbacks = server.callbacks();
}

void StrategyInstance::execOrder(bnc_order_t& order)
{
    m_server.execOrder(*this, order);
}

void StrategyInstance::deleteOrder(const bnc_order_t& order)
{
    m_server.deleteOrder(*this, order);
}

void StrategyInstance::deleteAllOrders()
{
    m_server.deleteAllOrders(*this);
}

void StrategyInstance::toLog(const std::string& msg)
{
    m_server.toLog(*this, msg);
}

void StrategyInstance::sendMessage(const std::string msg, bool alw)
{
    m_server.sendMessage(*this, msg, alw);
}

void StrategyInstance::requestValue(const std::string msg)
{
    m_server.requestValue(*this, msg);
}

void StrategyInstance::requestCommand(const std::string& msg, const python::list& list, TlgKeyboard::Side orientation)
{
    m_server.requestCommand(*this, msg, list, orientation);
}

void StrategyInstance::getPrice(const std::string& symbol)
{
    m_server.getPrice(*this, symbol);
}

void StrategyInstance::addStream(const std::string& stream)
{
    m_server.addStream(*this, stream);
}

void StrategyInstance::delStream(const std::string &stream)
{
    m_server.delStream(*this, stream);
}

void StrategyInstance::toFront(const std::string& symbol)
{
    m_server.tradeState(*this, symbol);
}

void StrategyInstance::toJournal(const std::string &data)
{
    m_server.toJournal(*this, data);
}

void StrategyInstance::toAlertChannel(const std::string &msg)
{
    m_server.toAlertChannel(*this, msg);
}

void StrategyInstance::createTimer(const std::string& name, python::object function)
{
    m_server.createTimer(*this, name, function);
}

void StrategyInstance::startTimer(const std::string& name, boost::uint64_t interval, bool recurring)
{
    m_server.startTimer(*this, name, interval, recurring);
}

void StrategyInstance::stopTimer(const std::string& name)
{
    m_server.stopTimer(*this, name);
}

void StrategyInstance::delTimer(const std::string& name)
{
    m_server.delTimer(*this, name);
}

const ss_callbacks_t& StrategyInstance::callbacks()
{
    return m_callbacks;
}

std::string StrategyInstance::format_error(python::handle<> const& exc, python::handle<> const& val, python::handle<> const& tb)
{
    if(m_format_exception.is_none())
        m_format_exception = python::import("traceback").attr("format_exception");
    return python::extract<std::string>(python::str("").join(m_format_exception(exc, val, tb)));
}


//-------------------------------------------------------------------------------------------

void StrategyServer::execOrder(StrategyInstance& instance, bnc_order_t& order)
{    
    instance.callbacks().on_create_order(order, OrderAction::OA_Execute);
}

void StrategyServer::deleteOrder(StrategyInstance& instance, const bnc_order_t& order)
{
    instance.callbacks().on_delete_order(order);
}

void StrategyServer::deleteAllOrders(StrategyInstance& instance)
{
    instance.callbacks().on_delete_all_orders();
}

void StrategyServer::toLog(StrategyInstance& instance, const std::string& msg)
{
    boost::ignore_unused(instance);
    mlogger::__info(msg.c_str());
}

void StrategyServer::sendMessage(StrategyInstance& instance, const std::string& msg, bool alw)
{
    instance.callbacks().on_send_message(msg, alw);
}

void StrategyServer::requestValue(StrategyInstance& instance, const std::string& msg)
{
    instance.callbacks().on_request_value(msg);
}

void StrategyServer::requestCommand(StrategyInstance& instance, const std::string& msg, const python::list& list, TlgKeyboard::Side orientation)
{
    tlg_keyboard_t tkb;

    tkb.msg = msg;
    tkb.orientation = orientation;

    for(int i = 0; i < len(list); ++i)
        tkb.keys.push_back(python::extract<std::string>(list[i]));

    instance.callbacks().on_request_command(tkb);
}

void StrategyServer::getPrice(StrategyInstance &instance, const std::string &symbol)
{
    instance.callbacks().on_get_price(symbol);
}

void StrategyServer::addStream(StrategyInstance &instance, const std::string &stream)
{
    instance.callbacks().on_add_stream(stream);
}

void StrategyServer::delStream(StrategyInstance &instance, const std::string &stream)
{
    instance.callbacks().on_del_stream(stream);
}

void StrategyServer::tradeState(StrategyInstance &instance, const std::string& data)
{
    instance.callbacks().on_trade_state(data);
}

void StrategyServer::toJournal(StrategyInstance &instance, const std::string& data)
{
    instance.callbacks().on_journal_data(data);
}

void StrategyServer::toAlertChannel(StrategyInstance &instance, const std::string &msg)
{
    instance.callbacks().on_alert_msg(msg);
}

void StrategyServer::createTimer(StrategyInstance& instance, const std::string& name, python::object function)
{
    instance.callbacks().on_create_timer(name, function);
}

void StrategyServer::startTimer(StrategyInstance& instance, const std::string& name, boost::uint64_t interval, bool recurring)
{
    instance.callbacks().on_start_timer(name, interval, recurring);
}

void StrategyServer::stopTimer(StrategyInstance& instance, const std::string& name)
{    
    instance.callbacks().on_stop_timer(name);
}

void StrategyServer::delTimer(StrategyInstance& instance, const std::string& name)
{
    instance.callbacks().on_del_timer(name);
}


std::string StrategyServer::format_error(StrategyInstance& instance, python::handle<> const& exc, python::handle<> const& val, python::handle<> const& tb)
{
    return instance.format_error(exc, val, tb);
}



