#ifndef WEBHOOK_SERVER_H
#define WEBHOOK_SERVER_H

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include <string_view>

#include "logger.h"

namespace ip = boost::asio::ip;         // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

typedef boost::function<void (const std::string&)> callback_webhook;

class wh_http_client : public boost::enable_shared_from_this<wh_http_client>, boost::noncopyable
{
public:
    typedef boost::shared_ptr<wh_http_client> pointer;

    wh_http_client(boost::asio::io_context& ioc, callback_webhook callback);
    void start(std::vector<pointer>* clients);
    void stop();

    inline tcp::socket& socket(){return m_socket;}
    static pointer create(boost::asio::io_context& io_context, callback_webhook callback)
    {
        return pointer(new wh_http_client(io_context, callback));
    }


private:
    void read_request();
    void process_request();
    void send_response(http::status status);

private:
    std::string m_name;
    tcp::socket m_socket;
    boost::beast::flat_static_buffer<8192> m_buffer;
    http::request<http::string_body> m_request;
    http::response<http::string_body> m_response;
    std::vector<pointer>* m_clients_list;
    callback_webhook m_callback;
};

class webhook_server
{
public:
    webhook_server(const std::string& name, callback_webhook callback) : m_name(name),
                                                                         m_callback(callback)
    {}
    ~webhook_server();

    inline void set_port(uint16_t port){m_port = port;}
    void start();


private:
    void start_accept();
    void on_accept(wh_http_client::pointer, const boost::beast::error_code& ec);

private:
    std::string m_name;
    boost::asio::io_context m_ioc{1};
    boost::thread m_ioc_thread;
    tcp::acceptor* m_acceptor = nullptr;
    std::vector<wh_http_client::pointer> clients;
    uint16_t m_port;
    callback_webhook m_callback;
};

#endif // WEBHOOK_SERVER_H
