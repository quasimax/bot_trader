#ifndef WRAPPER_H
#define WRAPPER_H

#pragma once

#include <list>
#include <string>
#include <boost/python.hpp>
#include <boost/python/wrapper.hpp>

#include "strategyserver.h"
#include "bnc_data_types.h"
#include "tkeyboard.h"

namespace python = boost::python;

class PyStrategyInstance final : public StrategyInstance, public python::wrapper<StrategyInstance>
{
    using StrategyInstance::StrategyInstance;

    void eval() override
    {
        get_override("eval")();
    }

    void onSymbolsList(python::list& symbols_list) override
    {
        get_override("on_symbols_list")(symbols_list);
    }

    void onOrderReport(const std::string& order_ex_report) override
    {
        get_override("on_order_report")(order_ex_report);
    }

    void onAlert(const std::string& name, const std::string& body) override
    {
        get_override("on_alert")(name, body);
    }

    void getIndicators() override
    {
        get_override("get_indicators")();
    }

    void onPrice(const std::string& data) override
    {
        get_override("on_price")(data);
    }

    void onWssData(const std::string& data) override
    {
        get_override("on_wss_data")(data);
    }

    void onAccount(const std::string& account) override
    {
        get_override("on_account")(account);
    }

    void onBalance(const std::string& account) override
    {
        get_override("on_balance")(account);
    }

    void onUserValue(const std::string& val) override
    {
        get_override("on_user_value")(val);
    }

    void onTradeData(const std::string& data) override
    {
        get_override("on_trade_data")(data);
    }

    void onGetTradeState(const std::string& symbol) override
    {
        get_override("on_get_trade_state")(symbol);
    }

    python::list getTradeSteps(const std::string& symbol) override
    {
        return get_override("get_trade_steps")(symbol);
    }

    void onTradeRun(const std::string& data) override
    {
        get_override("on_trade_run")(data);
    }

    void onExchangeInfo(const std::string& data) override
    {
        get_override("on_exchange_info")(data);
    }

    void getExchangeInfo() override
    {
        get_override("get_exchange_info")();
    }

};

bool init_strategy_interface(ss_callbacks_t& callbckas, const char* sprategy_path, python::object& strategy);


#endif // WRAPPER_H
