#ifndef BINANCE_H
#define BINANCE_H

#include <map>
#include <chrono>
#include <ctime>
#include <queue>
#include <stdlib.h>

#include <openssl/evp.h>
#include <openssl/hmac.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "boost/date_time/local_time/local_time.hpp"
#include <boost/asio/placeholders.hpp>


#include "wss_client.h"
#include "https_client.h"
#include "events.h"
#include "database.h"
#include "general.h"


class Binance
{
public:
    Binance(const std::string& name,
            asio::io_context& ioc,
            ssl::context& ctx,
            database& db,
            std::vector<std::string>& symbols_list,
            uinterface& intf,
            callback_event event_handler);

    ~Binance();

    void stop();

    inline void set_interface(uinterface intf){m_interface = intf;}
    inline uinterface interface(){return m_interface;}
    inline void set_recv_window(int rwind){m_recvWindow = rwind;}

    inline void set_WSS_ConnectionData(const std::string& host, short port)
    {
        m_wss_host = host;
        m_wss_port = port;
        m_wss_client_user.set_connection_data(host, port);
    }

    inline void set_HTTP_ConnectionData(const std::string& host, short port)
    {
        m_http_client.setConnectionData(host, port);
        m_target = host;
    }

    inline void set_API_keys(const std::string& api_key, const std::string& secret_key)
    {
        m_api_key = api_key;
        m_secret_key = secret_key;
    }

    void create_order(const bnc_order_t& order);
    void delete_order(const std::string& orderId, const std::string& symbol);
    void delete_all_orders();
    void get_open_orders(const std::string& symbol);
    void get_all_orders();
    void get_price(const std::string& symbol = "");
    void get_account();
    void get_exchange_info();

    void start_price_timer();

    void price_user_req()
    {
        mlogger::info("%s: Запрос текущего курса", m_name.c_str());
        if(0 == m_price_user_req)
            m_price_user_req = m_symbols_list.size();
    }

    void print_order(pt::ptree& pt, std::string& str);
    void print_order(const bnc_order_t& order, std::string& str);    
    void print_account(std::string& str);
    void print_execute_report(pt::ptree& pt, std::string& str);

    void add_wss_stream(const std::string& stream);
    void del_wss_stream(const std::string& stream);

private:
    void init();

    void create_user_stream();
    void ping_user_stream(const boost::system::error_code& ec);

    void on_connect_statistic_client(bool connect);
    void on_connect_http_client(bool connect);
    void on_connect_user_stream(bool connect);

    void on_price_timer(const boost::system::error_code& ec);
    void on_cleanup_timer(const boost::system::error_code& ec);

    void wss_statistic_handler(const std::string& data);
    void wss_user_handler(const std::string& data);
    void http_data_handler(http::request<http::string_body>& req, http::response<http::string_body>& resp);

    void parse_and_print_order_status(const pt::ptree& ppt, std::string& str);
    void parse_and_print_balance(const pt::ptree& ppt, std::string& str);
    void parse_and_print_balances(const pt::ptree& ppt, std::string& str);
    void parse_and_print_balance_upd(const pt::ptree& ppt, std::string& str);
    void parse_and_print_account(const pt::ptree& ppt, std::string& str);

    std::string urlenc_param(const std::string& urlstr, const std::string& param);    

private:
    std::string m_name;    
    ssl::context& m_ctx;
    asio::io_context& m_ioc;
    asio::strand<asio::io_context::executor_type> m_strand;
    callback_event m_event_handler;    
    std::string m_target;
    database& m_db;
    uinterface& m_interface;

    std::string m_api_key;
    std::string m_secret_key;
    std::string m_ustream_listen_key;

    std::map<std::string, WSS_Client*> m_wss_clients;

    WSS_Client m_wss_client_user{"Binance WSS client USER",
                            "",
                            m_ioc,
                            m_ctx,
                            boost::bind(&Binance::on_connect_user_stream, this, ::_1),
                            boost::bind(&Binance::wss_user_handler, this, ::_1)};

    HTTPS_Client m_http_client{"Binance HTTPS client",
                               m_ioc,
                               m_ctx,
                               boost::bind(&Binance::on_connect_http_client, this, ::_1),
                               boost::bind(&Binance::http_data_handler, this, ::_1, ::_2)};

    int m_price_user_req{0};
    int m_order_count{1};

    std::string m_wss_host;
    short m_wss_port;

    asio::deadline_timer m_ping_timer;
    asio::deadline_timer m_price_timer;
    asio::deadline_timer m_cleanup_timer;

    std::vector<std::string>& m_symbols_list;
    std::vector<std::string>::iterator m_price_it;

    int m_recvWindow{5000};

    uint64_t ttt;

};

#endif // BINANCE_H
