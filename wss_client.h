#ifndef WSS_CLIENT_H
#define WSS_CLIENT_H

#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/asio/strand.hpp>

#include "logger.h"

using tcp = boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;
namespace websocket = boost::beast::websocket;
namespace asio = boost::asio;

typedef boost::function<void (const std::string)> callback_receive;
typedef boost::function<void (bool)> callback_connect;

enum connect_state {Closed, WaitClosed, Connecting, Connected};

class WSS_Client : public std::enable_shared_from_this<WSS_Client>
{
public:
    WSS_Client(const std::string& name,
               const std::string& stream,
               asio::io_context& ioc,
               ssl::context& ctx,               
               callback_connect chandler,
               callback_receive mhandler);

    virtual ~WSS_Client();

    void connect();
    inline bool is_open(){return m_ws.is_open();}
    void close();
    void stop();
    inline connect_state state(){return m_connect_state;}

    inline void set_connection_data(const std::string& host, short port)
    {
        m_host = host;
        m_port = std::to_string(port);
    }

    inline void set_stream(const std::string& stream){m_stream = stream;}
    inline const std::string& stream(){return m_stream;}

private:
    void on_resolve(boost::system::error_code ec, tcp::resolver::results_type results);
    void on_connect(boost::system::error_code ec);
    void on_ssl_handshake(boost::system::error_code ec);
    void on_handshake(boost::system::error_code ec);
    void do_read();
    void on_read(boost::system::error_code ec, std::size_t bytes_transferred);
    void on_close(boost::system::error_code ec);

    static void result(char const* name, boost::system::error_code ec, char const* what)
    {
        if(!ec)
            mlogger::info("%s: %s - Ok", name, what);
        else
            mlogger::error("%s: %s - Err %s", name, what, ec.message().c_str());
    }

private:
    std::string m_name;    
    std::string m_stream;
    std::string m_host;
    std::string m_port;

    tcp::resolver m_resolver;
    websocket::stream<ssl::stream<tcp::socket>> m_ws;
    boost::asio::strand<boost::asio::executor> m_strand;
    boost::beast::multi_buffer m_buffer;
    std::string m_text;

    callback_receive m_data_handler;
    callback_connect m_connect_handler;

    //bool m_connected{false};
    //bool m_connecting{false};

    connect_state m_connect_state{connect_state::Closed};

};

#endif // WSS_CLIENT_H
