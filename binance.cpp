#include "binance.h"
#include "logger.h"
#include "utils.h"

Binance::Binance(const std::string& name,
                 asio::io_context& ioc,
                 ssl::context& ctx,
                 database& db,
                 std::vector<std::string>& symbols_list,
                 uinterface& intf,
                 callback_event event_handler) : m_name(name),                                                    
                                                 m_ctx(ctx),
                                                 m_ioc(ioc),
                                                 m_strand(boost::asio::make_strand(m_ioc)),
                                                 m_event_handler(event_handler),                                                 
                                                 m_db(db),
                                                 m_interface(intf),
                                                 m_ping_timer(ioc, boost::posix_time::seconds(1)),
                                                 m_price_timer(ioc, boost::posix_time::seconds(1)),
                                                 m_cleanup_timer(ioc, boost::posix_time::seconds(1)),
                                                 m_symbols_list(symbols_list)

{
    m_price_it = m_symbols_list.begin();
    m_http_client.setProxyData("noproxy");

    asio::deadline_timer init_timer(ioc, boost::posix_time::seconds(2));
    init_timer.async_wait(boost::bind(&Binance::init, this));
}

Binance::~Binance()
{
    stop();

    for(const auto& wssc : m_wss_clients)
        delete wssc.second;
}

void Binance::stop()
{
    m_ping_timer.cancel();
}

void Binance::init()
{
    mlogger::info("%s: Init", m_name.c_str());
    m_http_client.connect();    
}

void Binance::add_wss_stream(const std::string& stream)
{
    WSS_Client* wss_client = new WSS_Client("Binance WSS client " + stream,
                                           stream,
                                           m_ioc,
                                           m_ctx,
                                           boost::bind(&Binance::on_connect_statistic_client, this, ::_1),
                                           boost::bind(&Binance::wss_statistic_handler, this, ::_1));

    wss_client->set_connection_data(m_wss_host, m_wss_port);
    wss_client->connect();

    m_wss_clients.emplace(stream, wss_client);

    mlogger::info("Запущен новый WSS поток Binance: %s", stream.c_str());
}

void Binance::del_wss_stream(const std::string& stream)
{
    auto it = m_wss_clients.find(stream);
    if(m_wss_clients.end() != it)
    {
        delete it->second;
        m_wss_clients.erase(it);
        mlogger::info("Удаление WSS потока Binance: %s", stream.c_str());
    }
    else
    {
        mlogger::error("Попытка удаления не существующего WSS потока Binance: %s", stream.c_str());
    }
}

std::string Binance::urlenc_param(const std::string& urlstr, const std::string& param)
{
    std::size_t epos = 0;
    std::size_t spos = 0;

    spos = urlstr.find(param);
    spos = urlstr.find("=", spos++);
    epos = urlstr.find("&", spos);
    return urlstr.substr(spos, epos - spos);
}

void Binance::create_user_stream()
{
     mlogger::info("%s: Прередача запроса на создание Binance User Stream", m_name.c_str());

    std::map<std::string, std::string> xmap;
    xmap.insert(std::make_pair("X-MBX-APIKEY", m_api_key));

    m_http_client.request(http::verb::post,
                          "/api/v3/userDataStream",
                          HTTPS_Client::ContentType::nocontent,
                          pt::ptree(),
                          "",
                          xmap);
}

void Binance::ping_user_stream(const boost::system::error_code& ec)
{
    if(ec)
        return;

    std::map<std::string, std::string> xmap;
    xmap.insert(std::make_pair("X-MBX-APIKEY", m_api_key));

    pt::ptree pt;
    pt.put<std::string>("listenKey", m_ustream_listen_key);

    m_http_client.request(http::verb::put,
                          "/api/v3/userDataStream",
                          HTTPS_Client::ContentType::urlencoded,
                          pt,
                          "",
                          xmap);

    m_ping_timer.expires_from_now(boost::posix_time::seconds(1800));
    m_ping_timer.async_wait(asio::bind_executor(m_strand,
                                                boost::bind(&Binance::ping_user_stream,
                                                            this,
                                                            asio::placeholders::error)));
}

void Binance::start_price_timer()
{
    m_price_timer.expires_from_now(boost::posix_time::seconds(1));
    m_price_timer.async_wait(asio::bind_executor(m_strand,
                                                boost::bind(&Binance::on_price_timer,
                                                            this,
                                                            asio::placeholders::error)));

}

void Binance::on_price_timer(const boost::system::error_code& ec)
{
    if(ec)
        return;

    if(m_http_client.ready())
    {
        get_price();
        start_price_timer();
    }
}

void Binance::on_cleanup_timer(const boost::system::error_code& ec)
{
    if(ec)
        return;

    m_http_client.cleanup_requests(m_recvWindow);

    m_cleanup_timer.expires_from_now(boost::posix_time::milliseconds(m_recvWindow));
    m_cleanup_timer.async_wait(asio::bind_executor(m_strand,
                                                boost::bind(&Binance::on_cleanup_timer,
                                                            this,
                                                            asio::placeholders::error)));
}

void Binance::on_connect_statistic_client(bool connect)
{
    if(connect)
        mlogger::info("%s: Соединение WSS statistic stream - Ок", m_name.c_str());
    else
        mlogger::error("%s: Соединение WSS statistic stream - ERROR", m_name.c_str());
}

void Binance::on_connect_user_stream(bool connect)
{
    if(connect)
    {
        mlogger::info("%s: Соединение WSS user stream - Ок", m_name.c_str());
        ping_user_stream(make_error_code(boost::system::errc::success));
    }
    else
    {
        mlogger::error("%s: Соединение WSS user stream - ERROR", m_name.c_str());
        create_user_stream();
    }

}


void Binance::on_connect_http_client(bool connect)
{
    if(connect)
    {
        mlogger::info("%s: Соединение HTTPS client - Ок", m_name.c_str());

        get_account();
        get_exchange_info();
        start_price_timer();
        on_cleanup_timer(make_error_code(boost::system::errc::success));

        if(!m_wss_client_user.is_open())
        {
            create_user_stream();            
        }
    }
    else
    {
        m_price_timer.cancel();
        m_cleanup_timer.cancel();
        mlogger::error("%s: Соединение HTTPS client - ERROR", m_name.c_str());
    }

}

void Binance::wss_statistic_handler(const std::string& data)
{
    pt::ptree tree;
    pt::ptree subtree;
    std::istringstream bstr(data);    

    try
    {
        pt::read_json(bstr, tree);
    }
    catch (std::exception const& e)
    {
        mlogger::error("%s: Ошибка данных от Binance Stat Stream: %s\n%s", m_name.c_str(), e.what(), data.c_str());
        return;
    }

    pt::ptree::assoc_iterator it = tree.find("data");
    if(tree.not_found() == it)return;

    std::ostringstream oss;
    boost::property_tree::json_parser::write_json(oss, it->second);

    m_event_handler(Events::EVENT_BNC_WSS_DATA, boost::variant<std::string>(oss.str()));
}

void Binance::wss_user_handler(const std::string& data)
{
    pt::ptree tree;
    std::istringstream bstr(data);

    try
    {
        pt::read_json(bstr, tree);
    }
    catch (std::exception const& e)
    {
        mlogger::error("%s: Ошибка данных от Binance User Stream: %s\n%s", m_name.c_str(), e.what(), data.c_str());
        return;
    }

    mlogger::info("%s.%s: Получены данные от Binance User Stream:\n%s", m_name.c_str(),
                                                                        __func__,
                                                                        data.c_str());

    pt::ptree::assoc_iterator it = tree.find("data");
    if(tree.not_found() == it)return;

    std::string event = it->second.get<std::string>("e");
    std::string pstring;

    mlogger::info("%s: Получен отчет - %s:\n%s", m_name.c_str(), event.c_str(), data.c_str());

    if(/*"outboundAccountPosition" == event || */ "outboundAccountInfo" == event)
    {
        std::stringstream oss;
        boost::property_tree::json_parser::write_json(oss, it->second);
        pstring = oss.str();

        m_event_handler(Events::EVENT_BNC_BALANCE, boost::variant<std::string>(oss.str()));

        pstring.clear();        
        parse_and_print_balances(it->second.get_child("B"), pstring);
    }
    else if("balanceUpdate" == event)
    {
        parse_and_print_balance_upd(it->second, pstring);
    }
    else if("executionReport" == event)
    {
        std::ostringstream oss;
        boost::property_tree::json_parser::write_json(oss, it->second);
        m_event_handler(Events::EVENT_BNC_ORDER_REPORT, boost::variant<std::string>(oss.str()));

        std::string pstring;
        print_execute_report(it->second, pstring);
    }
    else if("listStatus" == event)
    {

    }
    else
        return;

    if(IntfTlg == m_interface)
        m_event_handler(Events::EVENT_TLG_TEXT_PRINT, boost::variant<std::string>(pstring));
}

void Binance::parse_and_print_account(const pt::ptree& ppt, std::string& str)
{
    str = "------------------------------\n";
    str += "ДАННЫЕ АККАУНТА:\n";
    str += "------------------------------\n";

    for(auto&& pv : ppt)
    {
        const std::string& key = pv.first;

        if("makerCommission" == key  || "m" == key)
            str += "Комиссия с продажи: " + pv.second.get_value<std::string>() + '\n';
        else if("takerCommission" == key || "t" == key)
            str += "Комиссия с покупки: " + pv.second.get_value<std::string>() + '\n';
        else if ("canTrade" == key || "T" == key)
            str += std::string("Может торговать: ") + (pv.second.get_value<bool>() ? "Да" : "Нет") + '\n';
        else if("canDeposit" == key || "D" == key)
            str += std::string("Может внести депозит: ") + (pv.second.get_value<bool>() ? "Да" : "Нет") + '\n';
        else if("canWithdraw" == key || "W" == key)
            str += std::string("Может снять: ") + (pv.second.get_value<bool>() ? "Да" : "Нет") + '\n';
        else if("balances" == key || "B" == key)
            parse_and_print_balance(pv.second, str);
    }
}

void Binance::parse_and_print_order_status(const pt::ptree& ppt, std::string& str)
{
    str = "------------------------------\n";

    for(auto&& pv : ppt)
    {
        const std::string& key = pv.first;

        if("clientOrderId" == key || "c" == key)
        {
            str += "ОРДЕР: " + pv.second.get_value<std::string>() + '\n';
            str += "------------------------------\n";
        }
        else if("symbol" == key || "s" == key)
            str += "пара: " + pv.second.get_value<std::string>() + '\n';
        else if("price" == key || "p" == key)
            str += "цена: " + pv.second.get_value<std::string>() + '\n';
        else if("origQty" == key || "Q" == key)
            str += "запрошено: " + pv.second.get_value<std::string>() + '\n';
        else if("executedQty" == key)
            str += "выполнено: " + pv.second.get_value<std::string>() + '\n';
        else if("status" == key  || "X" == key)
            str += "статус: " + pv.second.get_value<std::string>() + '\n';
        else if("timeInForce" == key || "f" == key)
            str += "вид: " + pv.second.get_value<std::string>() + '\n';
        else if("isWorking" == key)
            str += std::string("в работе: ") + (pv.second.get_value<bool>() ? "Да" : "Нет") + '\n';
        else if("type" == key || "o" == key)
            str += "тип: " + pv.second.get_value<std::string>() + '\n';
        else if("side" == key || "S" == key)
            str += "операция: " + pv.second.get_value<std::string>() + '\n';
    }

    str += "------------------------------\n";
}

void Binance::parse_and_print_balance(const pt::ptree& bpt, std::string& str)
{
    str += '\n';
    str += "Баланс:\n";
    str += "------------------------------\n";

    int counter = m_symbols_list.size() + 1;

    for(auto it = bpt.begin(); it != bpt.end(); ++it)
    {
        for(auto&& vv : it->second)
        {
            const std::string& key = vv.first;

            if("asset" == key || "a" == key)
            {
                const std::string& curr = vv.second.get_value<std::string>();
                bool symbol_in_list = std::find(m_symbols_list.begin(), m_symbols_list.end(), curr) != m_symbols_list.end();

                if("USDT" == curr || symbol_in_list)
                {
                    str += vv.second.get_value<std::string>() + '\n';
                    counter--;
                }
                else
                    break;
            }
            else if("free" == key || "f" == key)
                str += "свободно " + vv.second.get_value<std::string>() + "\n";
            else if("locked" == key || "l" == key)
                str += "блокировано " + vv.second.get_value<std::string>() + "\n";
        }

        if(0 == counter)
            break;
    }

    str += "------------------------------\n";
}

void Binance::parse_and_print_balances(const pt::ptree& bpt, std::string& str)
{
    str += '\n';
    str += "Баланс:\n";
    str += "------------------------------\n";

    for(auto it = bpt.begin(); it != bpt.end(); ++it)
    {
        for(auto&& vv : it->second)
        {
            const std::string& key = vv.first;

            if("a" == key)
              str += vv.second.get_value<std::string>() + '\n';
            else if("free" == key || "f" == key)
                str += "свободно " + vv.second.get_value<std::string>() + "\n";
            else if("locked" == key || "l" == key)
                str += "блокировано " + vv.second.get_value<std::string>() + "\n";
        }
    }

    str += "------------------------------\n";
}

void Binance::parse_and_print_balance_upd(const pt::ptree& ppt, std::string& str)
{
    std::string asset = ppt.get<std::string>("a", "???");
    double delta = ppt.get<double>("d", 0.0);
    uint64_t clear_time = ppt.get<uint64_t>("T", 0);

    str = "------------------------------\n";
    str += "БАЛАНС (Обновление):\n";
    str += "------------------------------\n";
    str += asset + ":" + '\n';
    str += "Дельта: " + std::to_string(delta) + '\n';
    str += "Срок: " + utils::local_time_from_ms(clear_time) + '\n';
    str += "------------------------------\n";
}

void Binance::http_data_handler(http::request<http::string_body>& req, http::response<http::string_body>& resp)
{
    pt::ptree tree;
    std::istringstream bstr(resp.body());
    bool print_body = true;

    try
    {
        pt::read_json(bstr, tree);
    }
    catch (std::exception const& e)
    {
        mlogger::error("%s: Binance parse json error: %s\n%s", m_name.c_str(), e.what(), resp.body().c_str());
        return;
    }    

    switch(resp.result())
    {
        case http::status::ok:
            break;

        case http::status::bad_request:
            HTTPS_Client::req_resp_to_log(req, resp);
            mlogger::warn("%s: HTTP 400 Bad Request %s", m_name.c_str(), resp.body().c_str());            
            mlogger::__error("Binance error: %s", resp.body().c_str());
            return;

        default:
            HTTPS_Client::req_resp_to_log(req, resp);
            mlogger::warn("BNC: Ошибка HTTP запроса code %d", resp.result());
            return;
    }

    if(std::string::npos != req.target().find("/api/v3/ping"))
    {
        return;
    }
    else if(std::string::npos != req.target().find("/api/v3/order"))
    {
        pt::ptree empt;
        std::ostringstream oss;

        empt.put<std::string>("e", "executionReport");
        empt.put<std::string>("s", tree.get<std::string>("symbol"));
        empt.put<std::string>("X", tree.get<std::string>("status"));
        empt.put<std::string>("c", tree.get<std::string>("clientOrderId"));
        empt.put<std::string>("f", tree.get<std::string>("timeInForce"));
        empt.put<std::string>("S", tree.get<std::string>("side"));
        empt.put<std::string>("o", tree.get<std::string>("type"));

        boost::property_tree::json_parser::write_json(oss, empt);
        m_event_handler(Events::EVENT_BNC_ORDER_REPORT, boost::variant<std::string>(oss.str()));
    }
    else if(std::string::npos != req.target().find("/api/v3/ticker/price"))
    {
        if(std::string::npos == req.target().find("symbol"))
        {
            std::string tgr = resp.body();

            for(auto& pit : tree.get_child(""))
            {
                std::string symb;
                std::string price;

                try
                {
                    symb = pit.second.get<std::string>("symbol");
                    price = pit.second.get<std::string>("price");
                }
                catch(std::exception& e)
                {
                    mlogger::error("%s-%s: HTTP price array - %s", m_name.c_str(), __func__, e.what());
                    continue;
                }

                if(std::string::npos == symb.find("USDT"))
                    continue;

                symb = symb.substr(0, symb.find("USDT"));
                if(m_symbols_list.end() == std::find(m_symbols_list.begin(), m_symbols_list.end(), symb))
                    continue;

                std::ostringstream oss;
                boost::property_tree::json_parser::write_json(oss, pit.second);
                m_event_handler(Events::EVENT_BNC_PRICE, boost::variant<std::string>(oss.str()));

                if(m_price_user_req)
                {
                    m_price_user_req--;
                    std::string msg = symb + "-USDT " + price;
                    m_event_handler(Events::EVENT_TLG_TEXT_PRINT, boost::variant<std::string>(msg));
                }
            }

            return;
        }
        else
        {
            m_event_handler(Events::EVENT_BNC_PRICE, boost::variant<std::string>(resp.body()));
            m_price_timer.cancel();
        }

        if(m_price_user_req)
        {
            m_price_user_req--;
            std::string msg = tree.get<std::string>("symbol") + " " + tree.get<std::string>("price");
            m_event_handler(Events::EVENT_TLG_TEXT_PRINT, boost::variant<std::string>(msg));
        }
    }
    else if(std::string::npos != req.target().find("/api/v3/account"))
    {
        m_event_handler(Events::EVENT_BNC_ACCOUNT, boost::variant<std::string>(resp.body()));

        std::string saccount;
        parse_and_print_account(tree, saccount);
        m_event_handler(Events::EVENT_TLG_TEXT_PRINT, boost::variant<std::string>(saccount));
        print_body = false;
    }
    else if(std::string::npos != req.target().find("/api/v3/exchangeInfo"))
    {
        m_event_handler(Events::EVENT_BNC_EXCHANGE_INFO, boost::variant<std::string>(resp.body()));
        print_body = false;
        return;
    }
    else if(std::string::npos != req.target().find("/api/v3/userDataStream"))
    {
        pt::ptree pt;
        std::map<std::string, std::string> xmap;

        switch(req.method())
        {
            case http::verb::put:  // это ping                
                return;

            case http::verb::post:  // получение ключа
                m_ustream_listen_key = tree.get<std::string>("listenKey");
                if(m_wss_client_user.state() == connect_state::Closed)
                {
                    m_wss_client_user.set_stream(m_ustream_listen_key);
                    m_wss_client_user.connect();
                }
                break;

            case http::verb::delete_:  // удаление ключа
                break;

            default:
                return;

        }
    }
    else if(std::string::npos != req.target().find("/api/v3/openOrders")
         || std::string::npos != req.target().find("/api/v3/allOrders"))
    {
        std::string msg;

        switch(req.method())
        {
            case http::verb::get:
                msg = "Cписок ордеров:";
                break;

            case http::verb::delete_:
                msg = "Список удаленых ордеров:";
                break;

            default:
                break;
        }

        m_event_handler(Events::EVENT_BNC_ORDER_LIST, boost::variant<std::string>(resp.body()));

        if(IntfTlg == m_interface)
        {
            m_event_handler(Events::EVENT_TLG_TEXT_PRINT, boost::variant<std::string>(msg));

            if(0 == tree.size())
            {
                msg = "Cписок ордеров пуст";
                m_event_handler(Events::EVENT_TLG_TEXT_PRINT, boost::variant<std::string>(msg));
                return;
            }

            for(auto it = tree.begin(); it != tree.end(); ++it)
            {
                parse_and_print_order_status(it->second, msg);
                m_event_handler(Events::EVENT_TLG_TEXT_PRINT, boost::variant<std::string>(msg));
            }
        }
    }

    HTTPS_Client::req_resp_to_log(req, resp, print_body);
}

void Binance::create_order(const bnc_order_t& order)
{
    mlogger::info("%s: Запрос на выставление ордера '%s'", m_name.c_str(), order.newClientOrderId.c_str());

    pt::ptree pt;
    pt.put("symbol", order.symbol);
    pt.put("side", BncOrder::side_to_string(order.side));
    pt.put("type", BncOrder::type_to_string(order.type));

    switch(order.type)
    {
        case BncOrder::Type::LIMIT:
            {
                pt.put("timeInForce", BncOrder::tif_to_string(order.timeInForce));
                pt.put("price", utils::double_to_string(order.price, 8));
                pt.put("quantity", utils::double_to_string(order.quantity, 8));
            }
            break;

        case BncOrder::Type::MARKET:
            pt.put("quantity", utils::double_to_string(order.quantity, 8));
/*
            if(0 == order.quantity)
                pt.put("quantity", order.quantity);
            else
                pt.put("quoteOrderQty", order.quoteOrderQty);
*/
            break;

        case BncOrder::Type::STOP_LOSS:
        case BncOrder::Type::TAKE_PROFIT:
            pt.put("quantity", order.quantity);
            pt.put("stopPrice", order.stopPrice);
            break;

        case BncOrder::Type::STOP_LOSS_LIMIT:
        case BncOrder::Type::TAKE_PROFIT_LIMIT:
            pt.put("timeInForce", BncOrder::tif_to_string(order.timeInForce));
            pt.put("quantity", utils::double_to_string(order.quantity, 8));
            pt.put("price", utils::double_to_string(order.price, 8));
            pt.put("stopPrice", order.stopPrice);
            break;

        case BncOrder::Type::LIMIT_MAKER:
            {
                pt.put("price", order.price);
                double _quantity = order.quantity;
                pt.put("quantity", utils::double_to_string(_quantity, 8));
            }
            break;

        default:
            return;
    }

    pt.put("newClientOrderId", order.newClientOrderId);
    pt.put("newOrderRespType", "RESULT");
    pt.put("timestamp", utils::curr_time_milliseconds());
    pt.put("recvWindow", m_recvWindow);

    std::stringstream ss;
    boost::property_tree::json_parser::write_json(ss, pt);

    mlogger::info("Создан ордер (вручную):\n%s", ss.str());

    std::map<std::string, std::string> xmap;
    xmap.insert(std::make_pair("X-MBX-APIKEY", m_api_key));

    m_http_client.request(http::verb::post,
                          "/api/v3/order",
                          HTTPS_Client::ContentType::urlencoded,
                          pt,
                          m_secret_key,
                          xmap);
}

void Binance::delete_order(const std::string& orderId, const std::string& symbol)
{
    mlogger::info("%s: Запрс на отмену ордера '%s'", m_name.c_str(), orderId.c_str());

    pt::ptree pt;
    pt.put("symbol", symbol);
    pt.put("origClientOrderId", orderId);
    pt.put("recvWindow", m_recvWindow);
    pt.put("timestamp", utils::curr_time_milliseconds());

    std::map<std::string, std::string> xmap;
    xmap.insert(std::make_pair("X-MBX-APIKEY", m_api_key));

    m_http_client.request(http::verb::delete_,
                          "/api/v3/order",
                          HTTPS_Client::ContentType::urlencoded,
                          pt,
                          m_secret_key,
                          xmap);
}

void Binance::delete_all_orders()
{
    mlogger::info("%s: Запрос на отмену всех открытых ордеров", m_name.c_str());

    std::map<std::string, std::string> xmap;
    xmap.insert(std::make_pair("X-MBX-APIKEY", m_api_key));

    m_http_client.request(http::verb::delete_,
                          "/api/v3/openOrders",
                          HTTPS_Client::ContentType::nocontent,
                          pt::ptree(),
                          m_secret_key,
                          xmap);
}

void Binance::print_order(pt::ptree& pt, std::string& str)
{
    str = "------------------------------\n";
    str += "ОРДЕР: " + pt.get<std::string>("newClientOrderId") + '\n';
    str += "------------------------------\n";
    str += "пара: " + pt.get<std::string>("symbol") + '\n';
    str += "операция: " + pt.get<std::string>("side") + '\n';
    str += "количество: " + pt.get<std::string>("quantity") + '\n';
    str += "цена: " + pt.get<std::string>("price") + '\n';
    str += "тип: " + pt.get<std::string>("type") + '\n';
    str += "вид: " + pt.get<std::string>("timeInForce") + '\n';
    str += "------------------------------\n";
}

void Binance::print_order(const bnc_order_t& order, std::string& str)
{
    str = "------------------------------\n";
    str += "ОРДЕР\n";
    str += "------------------------------\n";
    str += "имя: " + order.newClientOrderId + '\n';
    str += "пара: " + order.symbol + '\n';
    str += "операция: " + BncOrder::side_to_string(order.side) + '\n';
    str += "количество: " + utils::double_to_string(order.quantity, 5) + '\n';
    str += "цена: " + utils::double_to_string(order.price, 5) + '\n';
    str += "тип: " + BncOrder::type_to_string(order.type) + '\n';
    str += "вид: " + BncOrder::tif_to_string(order.timeInForce) + '\n';
    str += "------------------------------\n";
}

void Binance::print_execute_report(pt::ptree& pt, std::string& str)
{
    str = "------------------------------\n";
    str += "Отчет Binance:\n";
    str += "------------------------------\n";
    str += "имя: " + pt.get<std::string>("c") + '\n';
    str += "пара: " + pt.get<std::string>("s") + '\n';
    str += "операция: " + pt.get<std::string>("S") + '\n';
    str += "количество: " + pt.get<std::string>("q") + '\n';
    str += "цена: " + pt.get<std::string>("p") + '\n';
    str += "стоп цена: " + pt.get<std::string>("P") + '\n';
    str += "тип: " + pt.get<std::string>("f") + '\n';
    str += "вид: " + pt.get<std::string>("o") + '\n';
    str += "статус: " + pt.get<std::string>("X") + '\n';
    str += "Iceberg quantity: " + pt.get<std::string>("F") + '\n';
    str += "------------------------------\n";
}


void Binance::get_price(const std::string& symbol)
{
    pt::ptree pt;

    if(!symbol.empty())
        pt.put("symbol", symbol + "USDT");

    m_http_client.request(http::verb::get,
                          "/api/v3/ticker/price",
                          HTTPS_Client::ContentType::nocontent,
                          pt);
}

void Binance::get_account()
{
    mlogger::info("%s: Запрос данных аккаунта", m_name.c_str());

    pt::ptree pt;
    pt.put("recvWindow", m_recvWindow);
    pt.put("timestamp", std::to_string(utils::curr_time_milliseconds()));

    std::map<std::string, std::string> xmap;
    xmap.insert(std::make_pair("X-MBX-APIKEY", m_api_key));

    m_http_client.request(http::verb::get,
                          "/api/v3/account",
                          HTTPS_Client::ContentType::nocontent,
                          pt,
                          m_secret_key,
                          xmap);
}

void Binance::get_exchange_info()
{
    m_http_client.request(http::verb::get,
                          "/api/v3/exchangeInfo",
                          HTTPS_Client::ContentType::nocontent,
                          pt::ptree());
}

void Binance::get_open_orders(const std::string& symbol)
{
    mlogger::info("%s: Запрос на списка открытых ордеров", m_name.c_str());

    pt::ptree pt;
    pt.put("symbol", symbol);
    pt.put("recvWindow", m_recvWindow);
    pt.put("timestamp", std::to_string(utils::curr_time_milliseconds()));

    std::map<std::string, std::string> xmap;
    xmap.insert(std::make_pair("X-MBX-APIKEY", m_api_key));

    m_http_client.request(http::verb::get,
                          "/api/v3/openOrders",
                          HTTPS_Client::ContentType::nocontent,
                          pt,
                          m_secret_key,
                          xmap);
}

void Binance::get_all_orders()
{
    mlogger::info("%s: Запрос на всех ордеров", m_name.c_str());

    pt::ptree pt;
    //pt.put("symbol", "BTCUSDT");
    pt.put("timestamp", std::to_string(utils::curr_time_milliseconds()));

    std::map<std::string, std::string> xmap;
    xmap.insert(std::make_pair("X-MBX-APIKEY", m_api_key));

    m_http_client.request(http::verb::get,
                          "/api/v3/allOrders",
                          HTTPS_Client::ContentType::nocontent,
                          pt,
                          m_secret_key,
                          xmap);
}
