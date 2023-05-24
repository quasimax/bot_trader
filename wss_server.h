#ifndef WSS_SERVER_H
#define WSS_SERVER_H

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <list>
#include <map>

#include "events.h"
#include "logger.h"
#include "bnc_data_types.h"
#include "general.h"

namespace pt = boost::property_tree;
namespace asio = boost::asio;

#define PING_ATTEMPT    3
#define MAX_CONNECT     2

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;

class session;

typedef struct
{
    std::function<void (const std::shared_ptr<session>, boost::system::error_code&)> connect_handler;
    std::function<void (const std::shared_ptr<session>, boost::system::error_code&)> disconnect_handler;
    std::function<void (const std::shared_ptr<session>, const std::string&)> receive_handler;

} swi_callbacs_t;

class session : public std::enable_shared_from_this<session>
{
public:
    explicit session(tcp::socket socket, ssl::context& ctx, swi_callbacs_t& callbacks);
    ~session();
    void run();
    void close();
    void send(const std::string& data);
    void on_run();
    void on_handshake(boost::system::error_code ec);
    void on_accept(beast::error_code ec);
    void do_read();
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void on_send(const std::string& ss);
    void on_write(beast::error_code ec, std::size_t bytes_transferred);
    void on_close(beast::error_code ec);

    inline void set_ready_for_price(bool ready){m_ready_price = ready;}
    inline bool ready_for_price(){return m_ready_price;}

    inline void set_id(int id){m_id = id;}
    inline int id(){return m_id;}

private:
    void on_ping_timer(boost::system::error_code ec);
    void on_ping(boost::system::error_code ec);
    void on_control_callback(websocket::frame_type kind, boost::beast::string_view payload);

private:
    tcp::socket m_socket;
    websocket::stream<asio::ssl::stream<tcp::socket&>> m_ws;
    asio::strand<boost::asio::executor> m_strand;
    asio::deadline_timer m_ping_timer;
    int m_ping_counter{3};    
    std::deque<beast::multi_buffer> m_w_queue;
    beast::multi_buffer m_r_buffer;
    swi_callbacs_t& m_callbacks;
    int m_id{-1};
    bool m_ready_price{false};
};

//------------------------------------------------------------------------------


class listener : public std::enable_shared_from_this<listener>
{
    ssl::context& m_ctx;
    tcp::acceptor m_acceptor;
    tcp::socket m_socket;
    swi_callbacs_t& m_callbacks;
    bool m_ready{false};

public:
    listener(asio::io_context& ioc, ssl::context& ctx, tcp::endpoint endpoint, swi_callbacs_t& callbacks);
    inline void run(){do_accept();}    

private:
    void do_accept();
    void on_accept(beast::error_code ec);    
};

//------------------------------------------------------------------------------

class WSS_Server
{
public:
    WSS_Server(const std::string& name,
               boost::asio::io_context& ioc,
               uinterface& intf,
               std::vector<std::string>& symbols_list,
               callback_event event_handler);

    ~WSS_Server();

    inline void set_port(ushort port){m_port = port;}
    void set_certificate_chain_file(const std::string& fcert);
    void set_private_key_file(const std::string& fkey);
    inline void set_passwords(const std::string& adm_pass, const std::string& usr_pass)
    {
        m_admin_pssword = adm_pass;
        m_user_pssword = usr_pass;
    }    

    void run();
    void send(const pt::ptree& pt, int client = -2);
    void send(const std::string& data, int client = -2);

    void on_price(const std::string& data);
    void on_binance_error(const std::string& data);
    void on_account(const std::string& data);
    void on_orders_list(const std::string& data);
    void on_trade_steps(const std::string& symbol, const std::vector<std::string>& pos_list);

private:
    void on_connect(const std::shared_ptr<session> client, boost::system::error_code& ec);
    void on_disconnect(const std::shared_ptr<session> client, boost::system::error_code& ec);
    void on_receive(const std::shared_ptr<session> client, const std::string& msg);
    void change_client_key(const std::shared_ptr<session>, int key);
    void command_decoder(const std::shared_ptr<session> client, const std::string& cmd, const pt::ptree& pt);
    void error_message(const std::shared_ptr<session> client, int ecode);

    std::string decode64(const std::string &val);
    std::string encode64(const std::string &val);

private:
    asio::io_context& m_ioc;
    std::string m_admin_pssword{"123"};
    std::string m_user_pssword{"456"};
    std::multimap<int, std::shared_ptr<session>> m_client_map;
    ssl::context m_ctx{ssl::context::tlsv11_server};
    swi_callbacs_t m_callbacks;
    ushort m_port;
    uinterface& m_interface;
    int m_curr_id{1};
    std::vector<std::string>& m_symbols_list;

    std::string m_name;
    callback_event m_event_handler;
};

#endif // WSS_SERVER_H
