#ifndef APPLICATION_H
#define APPLICATION_H

#include <unistd.h>
#include <signal.h>

#include <iostream>
#include <string>
#include <atomic>
#include <memory>
#include <cstdlib>
#include <functional>
#include <memory>
#include <vector>

#include <boost/filesystem/fstream.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/string/split.hpp>

#include "logger.h"
#include "telegram.h"
#include "binance.h"
#include "events.h"
#include "database.h"
#include "webhook_server.h"
#include "py_wrapper.h"
#include "order.h"
#include "wss_server.h"
#include "statistic.h"
#include "timer.h"

using namespace std;
namespace fs = boost::filesystem;
namespace pt = boost::property_tree;

typedef struct
{
    fs::path conf_file_path;
    fs::path log_file_path;
    fs::path strategy_file_path;
    fs::path dbase_file_path;
    fs::path log_script_path;

} app_init_t;

class Application
{
public:
    Application(app_init_t& ain, asio::io_context& ioc);
    virtual ~Application();

    int init();

private:
     void read_config();
     void read_settings();
     void dispatcher();
     void add_event(Events event, const event_data_t& data);

     inline StrategyInstance& strategy()
     {
        return python::extract<StrategyInstance&>(m_strategy) BOOST_EXTRACT_WORKAROUND;
     }

     void python_error_to_log();

     void on_tradeview_webhook(const std::string& data);
     void on_script_message(const std::string& msg, bool alw);
     void on_script_request_value(const std::string& msg);
     void on_script_request_command(const tlg_keyboard_t& tkb);
     void on_script_order(const bnc_order_t& order, OrderAction act);
     void on_script_delete_order(const bnc_order_t& order);
     void on_script_delete_all_orders();
     void on_script_get_price(const std::string& symbol);
     void on_script_add_stream(const std::string& stream);
     void on_script_del_stream(const std::string& stream);
     void on_script_trade_state(const std::string& data);
     void on_script_journal_data(const std::string& data);
     void on_script_alert_msg(const std::string& msg);

     void on_event_tlg_menu(const event_data_t& data);
     void on_event_tlg_order(const event_data_t& data);
     void on_event_tlg_order_ops(const event_data_t& data);
     void on_event_tlg_script(const event_data_t& data);
     void on_event_tlg_get_price(const event_data_t& data);
     void on_event_tlg_test(const event_data_t& data);
     void on_event_tlg_print(const event_data_t& data);
     void on_event_tlg_update_id(const event_data_t& data);
     void on_event_tlg_get_account(const event_data_t& data);
     void on_event_tlg_get_indicators(const event_data_t& data);
     void on_event_tlg_get_statistic(const event_data_t& data);
     void on_event_tlg_order_data(const event_data_t& data);
     void on_event_tlg_script_data(const event_data_t& data);
     void on_event_tlg_stat_data(const event_data_t& data);

     void on_event_tdv_recv_alert(const event_data_t& data);
     void on_event_stat_ready(const event_data_t& data);

     void on_event_scr_create_order(const event_data_t& data);
     void on_event_scr_get_price(const event_data_t& data);
     void on_event_scr_add_stream(const event_data_t& data);
     void on_event_scr_del_stream(const event_data_t& data);
     void on_event_scr_req_command(const event_data_t& data);
     void on_event_scr_req_value(const event_data_t& data);
     void on_event_scr_trade_state(const event_data_t& data);
     void on_event_scr_get_filters(const event_data_t& data);

     void on_event_bnc_wss_data(const event_data_t& data);
     void on_event_bnc_account(const event_data_t& data);
     void on_event_bnc_price(const event_data_t& data);
     void on_event_bnc_balance(const event_data_t& data);
     void on_event_bnc_order_list(const event_data_t& data);
     void on_event_exchange_info(const event_data_t& data);

     void on_event_order_list(const event_data_t& data);
     void on_event_order_delete(const event_data_t& data);
     void on_event_order_report(const event_data_t& data);
     void on_event_order_man_result(const event_data_t& data);

     void on_event_web_set_trade_data(const event_data_t& data);
     void on_event_web_get_trade_state(const event_data_t& data);
     void on_event_web_req_trade_pos(const event_data_t& data);
     void on_event_web_trade_run(const event_data_t& data);

private:
    app_init_t& m_app_data;
    database m_db;
    std::vector<std::string> m_symbols_list;
    asio::io_context& m_ioc;
    asio::strand<asio::io_context::executor_type> m_strand;
    ssl::context m_ctx{ssl::context::sslv23_client};
    uinterface m_interface;

    Telegram m_telegram{"Telegram client",
                        m_ioc,
                        m_ctx,
                        m_interface,
                        m_symbols_list,
                        boost::bind(&Application::add_event, this, ::_1, ::_2)};

    Binance m_binance{"Binance client",
                      m_ioc,
                      m_ctx,
                      m_db,
                      m_symbols_list,
                      m_interface,
                      boost::bind(&Application::add_event, this, ::_1, ::_2)};

    TOrder m_order{m_telegram,
                   m_symbols_list,
                   boost::bind(&Application::add_event, this, ::_1, ::_2)};


    WSS_Server m_web_interface{"WEB WSS Interface",
                              m_ioc,
                              m_interface,
                              m_symbols_list,
                              boost::bind(&Application::add_event, this, ::_1, ::_2)};

    webhook_server m_webhook_server{"Tradeview server",
                                    boost::bind(&Application::on_tradeview_webhook, this, ::_1)};

    Statistic m_statistic{m_telegram,
                          m_symbols_list,
                          boost::bind(&Application::add_event, this, ::_1, ::_2)};

    event_list_t m_event_list;
    boost::mutex m_ev_mtx;
    std::map<Events, boost::function<void (const event_data_t&)>> event_handlers_map;
    python::object m_strategy;

    TimerMap m_py_timers{m_ioc};
};

#endif // APPLICATION_H
