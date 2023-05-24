#include "application.h"
#include "logger.h"
#include "general.h"
#include "events.h"

#include <iostream>
#include <chrono>
#include <thread>

#include <boost/asio/strand.hpp>

#define ERR_CONFIG_DATA     1
#define ERR_CONFIG_PATH     2
#define ERR_LOAD_SCRIPT     3

Application::Application(app_init_t& ain, asio::io_context& ioc) : m_app_data(ain),
                                                                   m_ioc(ioc),
                                                                   m_strand(boost::asio::make_strand(m_ioc))
{
    m_ctx.set_default_verify_paths();

    event_handlers_map.emplace(Events::EVENT_TLG_MENU,
                               boost::bind(&Application::on_event_tlg_menu, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_TLG_ORDER,
                               boost::bind(&Application::on_event_tlg_order, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_TLG_ORDER_OPS,
                               boost::bind(&Application::on_event_tlg_order_ops, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_TLG_GET_PRICE,
                               boost::bind(&Application::on_event_tlg_get_price, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_BNC_PRICE,
                               boost::bind(&Application::on_event_bnc_price, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_TLG_TEST,
                               boost::bind(&Application::on_event_tlg_test, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_TLG_TEXT_PRINT,
                               boost::bind(&Application::on_event_tlg_print, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_TLG_UPDATE_ID,
                               boost::bind(&Application::on_event_tlg_update_id, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_TLG_GET_ACCOUNT,
                               boost::bind(&Application::on_event_tlg_get_account, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_TLG_GET_INDICATORS,
                               boost::bind(&Application::on_event_tlg_get_indicators, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_TLG_STATISTIC,
                               boost::bind(&Application::on_event_tlg_get_statistic, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_TLG_STAT_DATA,
                               boost::bind(&Application::on_event_tlg_stat_data, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_TLG_SCRIPT,
                               boost::bind(&Application::on_event_tlg_script, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_TDV_ALERT,
                               boost::bind(&Application::on_event_tdv_recv_alert, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_STAT_READY,
                               boost::bind(&Application::on_event_stat_ready, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_TLG_ORDER_DATA,
                               boost::bind(&Application::on_event_tlg_order_data, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_BNC_WSS_DATA,
                               boost::bind(&Application::on_event_bnc_wss_data, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_BNC_ACCOUNT,
                               boost::bind(&Application::on_event_bnc_account, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_BNC_EXCHANGE_INFO,
                               boost::bind(&Application::on_event_exchange_info, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_BNC_BALANCE,
                               boost::bind(&Application::on_event_bnc_balance, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_BNC_ORDER_LIST,
                               boost::bind(&Application::on_event_bnc_order_list, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_ORD_DELETE,
                               boost::bind(&Application::on_event_order_delete, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_ORD_REQ_LIST,
                               boost::bind(&Application::on_event_order_list, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_BNC_ORDER_REPORT,
                               boost::bind(&Application::on_event_order_report, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_ORD_MANUAL,
                               boost::bind(&Application::on_event_order_man_result, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_TLG_SCRIPT_DATA,
                               boost::bind(&Application::on_event_tlg_script_data, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_SCR_GET_PRICE,
                               boost::bind(&Application::on_event_scr_get_price, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_SCR_ADD_STREAM,
                               boost::bind(&Application::on_event_scr_add_stream, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_SCR_DEL_STREAM,
                               boost::bind(&Application::on_event_scr_del_stream, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_SCR_REQ_COMMAND,
                               boost::bind(&Application::on_event_scr_req_command, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_SCR_REQ_VALUE,
                               boost::bind(&Application::on_event_scr_req_value, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_SCR_TRADE_STATE,
                               boost::bind(&Application::on_event_scr_trade_state, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_WEB_GET_FILTERS,
                               boost::bind(&Application::on_event_scr_get_filters, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_WEB_REQ_TRADE_STATE,
                               boost::bind(&Application::on_event_web_get_trade_state, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_WEB_SET_TRADE_DATA,
                               boost::bind(&Application::on_event_web_set_trade_data, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_WEB_REQ_TRADE_POS,
                               boost::bind(&Application::on_event_web_req_trade_pos, this, ::_1));

    event_handlers_map.emplace(Events::EVENT_WEB_TRADE_RUN,
                               boost::bind(&Application::on_event_web_trade_run, this, ::_1));

}

Application::~Application()
{
    m_db.close();
}

void Application::read_config()
{
    std::string msg = "Загрузка кофигурации из ";
    msg += m_app_data.conf_file_path.c_str();
    msg += " - ";
    int err = 0;

    try
    {
        pt::ptree pt;
        read_ini(m_app_data.conf_file_path.c_str(), pt);

        const boost::property_tree::ptree& general_conf = pt.get_child("general");

        std::string intf = general_conf.get<string>("interface", "telegram");

        if("web" == intf)
            m_interface = IntfWeb;
        else
            m_interface = IntfTlg;

        mlogger::info("Установлено управление от: %s", m_interface == IntfWeb ? "Web интерфейса" : "Telegram");

        m_app_data.log_script_path = general_conf.get<string>("script-log", string(DEF_LOG_DIR) + "trade.log");
        m_app_data.dbase_file_path = general_conf.get<string>("database", string(DEF_LOG_DIR) + "bibot.sqlite");

        const boost::property_tree::ptree& tlg_conf = pt.get_child("telegram");

        m_telegram.setConnectionData(tlg_conf.get<string>("host", "api.telegram.org"),
                                     tlg_conf.get<short>("port", 80),
                                     tlg_conf.get<string>("token", ""));

        m_telegram.setProxyData(tlg_conf.get<string>("proxy-mode", "none"),
                                tlg_conf.get<string>("proxy-host", "127.0.0.1"),
                                tlg_conf.get<string>("proxy-port", "9050"));

        m_telegram.set_alarm_channel_id(tlg_conf.get<std::string>("alert-channel-id", ""));

        const boost::property_tree::ptree& bnc_conf = pt.get_child("binance");

        m_binance.set_WSS_ConnectionData(bnc_conf.get<string>("wss-host", "stream.binance.com"),
                                         bnc_conf.get<short>("port", 9443));

        m_binance.set_HTTP_ConnectionData(bnc_conf.get<string>("http-host", "api.binance.com"),
                                          bnc_conf.get<short>("http-port", 80));

        m_binance.set_API_keys(bnc_conf.get<string>("api-key", ""),
                               bnc_conf.get<string>("secret-key", ""));

        m_binance.set_recv_window(bnc_conf.get<int>("recv-window", 5000));

        std::string symbols = bnc_conf.get<string>("symbols", "");
        boost::algorithm::split(m_symbols_list, symbols, boost::is_any_of(","));        

        const boost::property_tree::ptree& web_conf = pt.get_child("web");

        m_web_interface.set_port(web_conf.get<ushort>("port", 7443));
        m_web_interface.set_certificate_chain_file(web_conf.get<std::string>("certificate-chain-file", "server.pem"));
        m_web_interface.set_private_key_file(web_conf.get<std::string>("private-key-file", "privkey.pem"));
        m_web_interface.set_passwords(web_conf.get<std::string>("control-password", ""),
                                      web_conf.get<std::string>("observer-password", ""));

        const boost::property_tree::ptree& ss_conf = pt.get_child("strategy");
        m_app_data.strategy_file_path = ss_conf.get<string>("path", "");

        const boost::property_tree::ptree& wh_conf = pt.get_child("tradeview");
        m_webhook_server.set_port(wh_conf.get<short>("webhook-port", 8000));
    }
    catch (const pt::ptree_bad_data& error)
    {
        mlogger::error("%s: %s", __func__, error.what());
        err = ERR_CONFIG_DATA;
    }
    catch (const pt::ptree_bad_path& error)
    {
        mlogger::error("%s: %s", __func__, error.what());
        err = ERR_CONFIG_PATH;
    }

    if(0 == err)
    {
        msg += "Ok";
        mlogger::info(msg.c_str());
    }
    else
    {
        msg += "Ошибка";
        mlogger::error(msg.c_str());
        exit(err);
    }


}

void Application::read_settings()
{
    std::string sql = "SELECT * FROM settings";
    std::string msg = "%s: Загрузка сохраненного состояния из БД - ";

    if(m_db.exec(sql) && m_db.next())
    {
        m_telegram.set_settings(m_db.get_value<int>("last_update_id", 0));
        msg += "Ok";
    }
    else
    {
        msg += "Ошибка";
    }

    mlogger::error("%s", msg.c_str());
}

void Application::add_event(Events event, const event_data_t& data)
{
    m_ev_mtx.lock();
    m_event_list.push_back(std::make_pair(event, data));
    m_ev_mtx.unlock();

    m_ioc.post(asio::bind_executor(m_strand, boost::bind(&Application::dispatcher, this)));
}

void Application::dispatcher()
{
    while(m_event_list.size())
    {
        auto handler = event_handlers_map.find(m_event_list.front().first);
        if(event_handlers_map.end() != handler)
        {
            handler->second(m_event_list.front().second);
        }

        m_ev_mtx.lock();
        m_event_list.pop_front();
        m_ev_mtx.unlock();
    }
}

void Application::python_error_to_log()
{
    PyObject *exc, *val, *tb;
    PyErr_Fetch(&exc, &val, &tb);
    PyErr_NormalizeException(&exc, &val, &tb);
    python::handle<> hexc(exc), hval(python::allow_null(val)), htb(python::allow_null(tb));
    std::string emessage = python::extract<string>(!hval ? python::str(hexc) : python::str(hval));
    std::string edetails = strategy().format_error(hexc, hval, htb);

    mlogger::error("Pythyon error: %s", emessage.c_str());
    mlogger::error("Pythyon details: %s", edetails.c_str());

}

int Application::init()
{
    read_config();

    if(!m_db.open(m_app_data.dbase_file_path.c_str()))
    {
        mlogger::error("%s: Ошибка доступа к БД: %s", __func__, m_app_data.dbase_file_path.c_str());
    }

    read_settings();

    ss_callbacks_t script_callbacks;
    script_callbacks.on_send_message = boost::bind(&Application::on_script_message, this, ::_1, ::_2);
    script_callbacks.on_request_value = boost::bind(&Application::on_script_request_value, this, ::_1);
    script_callbacks.on_request_command = boost::bind(&Application::on_script_request_command, this, ::_1);
    script_callbacks.on_create_order = boost::bind(&Application::on_script_order, this, ::_1, ::_2);
    script_callbacks.on_delete_order = boost::bind(&Application::on_script_delete_order, this, ::_1);
    script_callbacks.on_delete_all_orders = boost::bind(&Application::on_script_delete_all_orders, this);
    script_callbacks.on_get_price = boost::bind(&Application::on_script_get_price, this, ::_1);
    script_callbacks.on_add_stream = boost::bind(&Application::on_script_add_stream, this, ::_1);
    script_callbacks.on_del_stream = boost::bind(&Application::on_script_del_stream, this, ::_1);
    script_callbacks.on_trade_state = boost::bind(&Application::on_script_trade_state, this, ::_1);
    script_callbacks.on_journal_data = boost::bind(&Application::on_script_journal_data, this, ::_1);
    script_callbacks.on_alert_msg = boost::bind(&Application::on_script_alert_msg, this, ::_1);

    script_callbacks.on_create_timer = boost::bind(&TimerMap::add_timer, boost::ref(m_py_timers), ::_1, ::_2);
    script_callbacks.on_start_timer = boost::bind(&TimerMap::start_timer, boost::ref(m_py_timers), ::_1, ::_2, ::_3);
    script_callbacks.on_stop_timer = boost::bind(&TimerMap::stop_timer, boost::ref(m_py_timers), ::_1);
    script_callbacks.on_del_timer = boost::bind(&TimerMap::del_timer, boost::ref(m_py_timers), ::_1);


    if(!init_strategy_interface(script_callbacks, m_app_data.strategy_file_path.c_str(), m_strategy))
    {
        mlogger::error("%s: Загрузка торгового скрипта - Ошибка", __func__);
        return ERR_LOAD_SCRIPT;
    }

    mlogger::info("%s: Загрузка торгового скрипта - Оk", __func__);
    mlogger::init_script_log(m_app_data.log_script_path.c_str());

    boost::python::list py_symbols_list;
    for(const auto& it : m_symbols_list)
        py_symbols_list.append(it);

    try
    {
        strategy().onSymbolsList(py_symbols_list);
    }
    catch(python::error_already_set const&)
    {
        python_error_to_log();
    }

    //m_webhook_server.start();
    mlogger::info("%s: Запуск Web сервера", __func__);
    m_web_interface.run();

    return EXIT_SUCCESS;
}


void Application::on_script_message(const std::string& msg, bool alw)
{
    if(alw || IntfTlg == m_interface)
        add_event(Events::EVENT_TLG_TEXT_PRINT, boost::variant<std::string>(msg));
}

void Application::on_script_request_value(const std::string& msg)
{
    add_event(Events::EVENT_SCR_REQ_VALUE, boost::variant<std::string>(msg));
}

void Application::on_script_request_command(const tlg_keyboard_t& tkb)
{
    add_event(Events::EVENT_SCR_REQ_COMMAND, boost::variant<tlg_keyboard_t>(tkb));
}

void Application::on_script_order(const bnc_order_t& order, OrderAction act)
{
    std::string str;

    m_binance.print_order(order, str);

    switch(act)
    {
        case OA_View:
            m_telegram.send_message(m_telegram.chat_id(), str);
            break;

        case OA_Execute:
            m_binance.create_order(order);
            break;

        default:
            break;
    }
}

void Application::on_script_delete_order(const bnc_order_t& order)
{
    add_event(Events::EVENT_ORD_DELETE, boost::variant<bnc_order_t>(order));
}

void Application::on_script_delete_all_orders()
{
    m_binance.delete_all_orders();
}

void Application::on_script_get_price(const std::string& symbol)
{
    add_event(Events::EVENT_SCR_GET_PRICE, boost::variant<std::string>(symbol));
}

void Application::on_script_add_stream(const std::string& stream)
{
    add_event(Events::EVENT_SCR_ADD_STREAM, boost::variant<std::string>(stream));
}

void Application::on_script_del_stream(const std::string& stream)
{
    add_event(Events::EVENT_SCR_DEL_STREAM, boost::variant<std::string>(stream));
}

void Application::on_script_trade_state(const std::string& data)
{
    add_event(Events::EVENT_SCR_TRADE_STATE, boost::variant<std::string>(data));
}

void Application::on_script_journal_data(const std::string& data)
{
    ulong t = std::time(0);
    std::ostringstream osq;
    ulong ldt = t - 60 * 60 * 24;

    std::vector<std::string> jdata;
    boost::algorithm::split(jdata, boost::get<std::string>(data), boost::is_any_of("@"));

    osq << "DELETE FROM journal WHERE ";
    osq << "dt < datetime(" << ldt << ", 'unixepoch', 'localtime')";
    m_db.exec(osq.str());

    osq.str("");
    osq.clear();

    osq << "INSERT INTO journal (dt, symbol, message) VALUES (";
    osq << "datetime(" << t << ", 'unixepoch', 'localtime'),";
    osq << "'" + jdata.at(0) + "',";
    osq << "'" + jdata.at(1) + "')";

    std::string tmp = osq.str();

    m_db.exec(osq.str());
}

void Application::on_script_alert_msg(const std::string& msg)
{
    m_telegram.send_alarm_message(msg);
}

void Application::on_tradeview_webhook(const std::string& data)
{
    //add_event(Events::EVENT_TDV_ALERT, boost::variant<std::string>(data));
    try
    {
        strategy().onAlert("ALERT", std::string(data));
    }
    catch(python::error_already_set const &)
    {
            python_error_to_log();
    }
}

void Application::on_event_tlg_menu(const event_data_t& data)
{
    boost::ignore_unused(data);

    TlgKeyboard keyboard;
    std::vector<std::string> kv;

    kv.push_back("/account - данные аккаунта");
    kv.push_back("/price - текущий курс валют");
    if(IntfTlg == m_interface)
    {
        kv.push_back("/order - операции с ордерами");
        kv.push_back("/script - управление скриптом");
    }
//    kv.push_back("/indicators - данные индикаторов");
    kv.push_back("/stat - журнал операций");
    kv.push_back("/cancel - отмена");
    keyboard.add_keys(kv, TlgKeyboard::Side::Vertical);

    m_telegram.send_message(m_telegram.chat_id(), "menu", "", Telegram::MessageMode::Keyboard, keyboard.keyboard());
}

void Application::on_event_tlg_order(const event_data_t& data)
{
    boost::ignore_unused(data);

    if(IntfTlg != m_interface)
        return;

    TlgKeyboard keyboard;
    std::vector<std::string> kv;

    kv.push_back("/order new - создать ордер");
    kv.push_back("/order list - список ордеров");
    kv.push_back("/order open - список открытых ордеров");
    kv.push_back("/order del - удалить ордер");
    kv.push_back("/menu - назад");
    keyboard.add_keys(kv, TlgKeyboard::Side::Vertical);

    m_telegram.send_message(m_telegram.chat_id(), "Операции с ордерами:", "", Telegram::MessageMode::Keyboard, keyboard.keyboard());
}

void Application::on_event_tlg_order_ops(const event_data_t& data)
{
    if(IntfTlg != m_interface)
        return;

    bnc_command_t cmd = boost::get<bnc_command_t>(data);

    switch(cmd.cmd)
    {
        case bnc_command::LIST:
            if(bnc_type::OPENED == cmd.type)
                m_order.list();
            else if(bnc_type::ALL == cmd.type)
                m_binance.get_all_orders();
            break;

        case bnc_command::INFO:
            break;

        case bnc_command::DELETE:
            m_order.del();
            break;

        case bnc_command::NEW:
            m_order.start();
            break;

        default:
            break;
    }
}

void Application::on_event_tlg_script(const event_data_t& data)
{
    std::string arg = boost::get<std::string>(data);

    if(IntfTlg != m_interface)
        return;

    if("start" == arg)
    {
/*
        try
        {
            strategy().onStart();
        }
        catch(python::error_already_set const&)
        {
            python_error_to_log();
        }
    }
    else if("data" == arg)
    {
        try
        {
            strategy().onChange();
        }
        catch(python::error_already_set const&)
        {
            python_error_to_log();
        }
    }
    else if("stop" == arg)
    {
        try
        {
            strategy().onStop();
        }
        catch(python::error_already_set const&)
        {
            python_error_to_log();
        }
    }
    else
    {

        if(!strategy().isScriptRunning())
            kv.push_back("/script start - запустить скрипт");
        else
            kv.push_back("/script data - изменить данные");

        kv.push_back("/script stop - остановить скрипт");
*/
        TlgKeyboard keyboard;
        std::vector<std::string> kv;

        kv.push_back("/menu - назад");
        keyboard.add_keys(kv, TlgKeyboard::Side::Vertical);

        m_telegram.send_message(m_telegram.chat_id(), "Операции со скриптом:", "", Telegram::MessageMode::Keyboard, keyboard.keyboard());
    }
}

void Application::on_event_tlg_get_price(const event_data_t& data)
{
    boost::ignore_unused(data);
    m_binance.price_user_req();
    m_binance.get_price();
}

void Application::on_event_bnc_price(const event_data_t& data)
{
    try
    {
        strategy().onPrice(boost::get<std::string>(data));
    }
    catch(python::error_already_set const&)
    {
        python_error_to_log();
    }

    m_web_interface.on_price(boost::get<std::string>(data));
}

void Application::on_event_tlg_print(const event_data_t& data)
{
    m_telegram.send_message(m_telegram.chat_id(), boost::get<std::string>(data));
}

void Application::on_event_tlg_update_id(const event_data_t& data)
{
    std::string sq = "UPDATE settings SET last_update_id = " +
                     std::to_string(boost::get<int>(data));
    m_db.exec(sq);
}

void Application::on_event_tlg_get_account(const event_data_t& data)
{
    boost::ignore_unused(data);
    m_binance.get_account();
}

void Application::on_event_tdv_recv_alert(const event_data_t& data)
{
    try
    {
        strategy().onAlert("", boost::get<std::string>(data));
    }
    catch(python::error_already_set const&)
    {
        python_error_to_log();
    }
}

void Application::on_event_stat_ready(const event_data_t& data)
{
    const stat_data_t& sd(boost::get<stat_data_t>(data));
    std::string sq;
    sq = "SELECT * FROM journal";

    if(!sd.symbol.empty())
    {
        sq += " WHERE symbol='" + sd.symbol + "'";
    }

    if(0 != sd.hours)
    {
        sq += std::string::npos == sq.find("WHERE") ? " WHERE " : " AND ";
        ulong t = std::time(0);
        t -= sd.hours * 60 * 60;
        sq += "dt > datetime(" + std::to_string(t) + ", 'unixepoch', 'localtime')";
    }

    if(!m_db.exec(sq))
    {
        m_telegram.send_message(m_telegram.chat_id(), "Ошибка доступа к БД");
        return;
    }

    std::string mdata;
    std::string temp;
    int count = 0;

    mdata += "Статистика торгов:\n";
    mdata += "----------------------------------\n";

    while(m_db.next())
    {
        count++;

        temp = m_db.get_value<std::string>("dt") + " ";
        temp += m_db.get_value<std::string>("symbol") + " ";
        temp += m_db.get_value<std::string>("message") + "\n";

        int tsize = temp.size() * sizeof(std::string::value_type);
        int dsize = mdata.size() * sizeof(std::string::value_type);

        if(MAX_TEXT_MSG_SIZE < tsize + dsize)
        {
            m_telegram.send_message(m_telegram.chat_id(), mdata);
            mdata.clear();
        }

        mdata += temp;
    }

    if(0 == count)
        mdata += "Нет записей";

    m_telegram.send_message(m_telegram.chat_id(), mdata);
}

void Application::on_event_tlg_get_indicators(const event_data_t& data)
{
    boost::ignore_unused(data);

    try
    {
        strategy().getIndicators();
    }
    catch(python::error_already_set const&)
    {
        python_error_to_log();
    }
}

void Application::on_event_tlg_get_statistic(const event_data_t& data)
{
    boost::ignore_unused(data);
    m_statistic.start();
}

void Application::on_event_scr_create_order(const event_data_t& data)
{
    m_binance.create_order(boost::get<bnc_order_t>(data));
}

void Application::on_event_scr_get_price(const event_data_t& data)
{
    m_binance.get_price(boost::get<std::string>(data));
}

void Application::on_event_scr_add_stream(const event_data_t& data)
{
    m_binance.add_wss_stream(boost::get<std::string>(data));
}

void Application::on_event_scr_del_stream(const event_data_t& data)
{
    m_binance.del_wss_stream(boost::get<std::string>(data));
}

void Application::on_event_scr_req_command(const event_data_t& data)
{
    tlg_keyboard_t tkb = boost::get<tlg_keyboard_t>(data);

    TlgKeyboard keyboard;
    keyboard.add_keys(tkb.keys, tkb.orientation);
    m_telegram.send_message(m_telegram.chat_id(), tkb.msg, "", Telegram::MessageMode::Keyboard, keyboard.keyboard());
    m_telegram.set_input_mode(Telegram::IM_Script);
}

void Application::on_event_scr_req_value(const event_data_t& data)
{
    m_telegram.send_message(m_telegram.chat_id(),
                            boost::get<std::string>(data),
                            "",
                            Telegram::Replay);
    m_telegram.set_input_mode(Telegram::IM_Script);
}

void Application::on_event_scr_trade_state(const event_data_t& data)
{
    m_web_interface.send(boost::get<std::string>(data));
}

void Application::on_event_scr_get_filters(const event_data_t& data)
{
    boost::ignore_unused(data);

    try
    {
        strategy().getExchangeInfo();
    }
    catch(python::error_already_set const&)
    {
        python_error_to_log();
    }
}

void Application::on_event_tlg_order_data(const event_data_t& data)
{
    m_order.set_data(boost::get<std::string>(data));
}

void Application::on_event_tlg_script_data(const event_data_t& data)
{
    try
    {
        strategy().onUserValue(boost::get<std::string>(data));
    }
    catch(python::error_already_set const&)
    {
        python_error_to_log();
    }
}

void Application::on_event_tlg_stat_data(const event_data_t& data)
{
    m_statistic.set_data(boost::get<std::string>(data));
}

void Application::on_event_bnc_wss_data(const event_data_t& data)
{
    try
    {
        strategy().onWssData(boost::get<std::string>(data));
    }
    catch(python::error_already_set const&)
    {
        python_error_to_log();
    }
}

void Application::on_event_bnc_account(const event_data_t& data)
{
    try
    {       
        strategy().onAccount(boost::get<std::string>(data));
    }
    catch(python::error_already_set const&)
    {
        python_error_to_log();
    }

    m_web_interface.on_account(boost::get<std::string>(data));
}

void Application::on_event_exchange_info(const event_data_t& data)
{
    try
    {
        strategy().onExchangeInfo(boost::get<std::string>(data));
    }
    catch(python::error_already_set const&)
    {
        python_error_to_log();
    }
}

void Application::on_event_bnc_balance(const event_data_t& data)
{
    try
    {
        strategy().onBalance(boost::get<std::string>(data));
    }
    catch(python::error_already_set const&)
    {
        python_error_to_log();
    }

    m_web_interface.send(boost::get<std::string>(data));
}

void Application::on_event_bnc_order_list(const event_data_t& data)
{
    m_web_interface.on_orders_list(boost::get<std::string>(data));
}

void Application::on_event_order_delete(const event_data_t& data)
{
    bnc_order_t order = boost::get<bnc_order_t>(data);
    if("*" == order.newClientOrderId)
        m_binance.delete_all_orders();
    else
        m_binance.delete_order(order.newClientOrderId, order.symbol);
}

void Application::on_event_order_report(const event_data_t& data)
{
    pt::ptree tree;
    std::istringstream bstr(boost::get<std::string>(data));

    try
    {
        pt::read_json(bstr, tree);
    }
    catch (std::exception const& e)
    {
        mlogger::error("Application: Binance parse report json error:%s\n%s", e.what(), bstr.str().c_str());
        return;
    }

    std::string msg;

    if(tree.not_found() != tree.find("code"))
    {
        msg = "Ошибка ордера Binance, код ";
        msg += tree.get<std::string>("code") + '\n';
        msg += "Диагноз: " + tree.get<std::string>("msg");
        m_web_interface.on_binance_error(bstr.str());
    }
    else
    {
        msg = "Ордер принят Binance";
        m_web_interface.send(bstr.str());
    }

    m_telegram.send_message(m_telegram.chat_id(), msg);

    try
    {
        strategy().onOrderReport(bstr.str());
    }
    catch(python::error_already_set const &)
    {
        python_error_to_log();
    }
}

void Application::on_event_order_list(const event_data_t& data)
{
    std::string symbol = boost::get<std::string>(data);

    if(symbol.empty())
        m_binance.get_all_orders();
    else
        m_binance.get_open_orders(boost::get<std::string>(symbol + "USDT"));
}

void Application::on_event_order_man_result(const event_data_t& data)
{
    OrderResult ores = boost::get<OrderResult>(data);

    switch(ores)
    {
        case OST_Execute:
            break;

        default:
            return;
    }

    m_binance.create_order(m_order.get());
}

//==================================================================================

void Application::on_event_tlg_test(const event_data_t& data)
{
    boost::ignore_unused(data);

    try
    {
        strategy().eval();
    }
    catch(python::error_already_set const& e)
    {
        //python_error_to_log();
    }
}

void Application::on_event_web_set_trade_data(const event_data_t& data)
{
    try
    {
        strategy().onTradeData(boost::get<std::string>(data));
    }
    catch(python::error_already_set const &)
    {
        python_error_to_log();
    }
}

void Application::on_event_web_get_trade_state(const event_data_t& data)
{
    try
    {
        strategy().onGetTradeState(boost::get<std::string>(data));
    }
    catch(python::error_already_set const &)
    {
        python_error_to_log();
    }
}

void Application::on_event_web_req_trade_pos(const event_data_t& data)
{
    std::vector<std::string> pos_list;
    python::list plist;


    try
    {
        plist = strategy().getTradeSteps(boost::get<std::string>(data));
        for(int i = 0; i < len(plist); ++i)
            pos_list.push_back(boost::python::extract<std::string>(plist[i]));

        m_web_interface.on_trade_steps(boost::get<std::string>(data), pos_list);
    }
    catch(python::error_already_set const &)
    {
        python_error_to_log();
    }
}

void Application::on_event_web_trade_run(const event_data_t& data)
{
    try
    {
        strategy().onTradeRun(boost::get<std::string>(data));
    }
    catch(python::error_already_set const &)
    {
        python_error_to_log();
    }
}

