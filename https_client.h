#ifndef HTTPS_CLIENT_H
#define HTTPS_CLIENT_H

#include <iostream>
#include <fstream>
#include <queue>
#include <regex>

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "sock4a.hpp"
#include "logger.h"

using tcp = boost::asio::ip::tcp;               // from <boost/asio/ip/tcp.hpp>
namespace ssl = boost::asio::ssl;               // from <boost/asio/ssl.hpp>
namespace beast = boost::beast;                 // from <boost/beast.hpp>
namespace asio = boost::asio;                   // from <boost/asio.hpp>
namespace http = beast::http;                   // from <boost/beast/http.hpp>
namespace pt = boost::property_tree;



typedef boost::function<void (http::request<http::string_body>& req, http::response<http::string_body>& resp)> callback_response;
typedef boost::function<void (bool)> callback_connect;
typedef struct
{
    uint64_t push_time;
    uint32_t send_time;
    http::request<http::string_body> request;
    http::response<http::string_body> response;

} http_req_resp_t;

typedef std::list<http_req_resp_t> http_req_resp_list;

class HTTPS_Client// : std::enable_shared_from_this<HTTPS_Client>
{
public:    
    enum ProxyMode{noproxy, http, socks4a};
    enum ContentType{nocontent, text_palin, json, urlencoded};

    HTTPS_Client(const std::string& name,
                 asio::io_context& ioc,
                 ssl::context& ctx,
                 callback_connect chandler,
                 callback_response mhandler);

    virtual ~HTTPS_Client();

    void connect();
    void stop();
    inline bool ready(){return m_ready;}

    void request(http::verb method,
                 const std::string& target,
                 ContentType content_type,
                 const pt::ptree& payload = pt::ptree(),
                 const std::string& secret_key = "",
                 const std::map<std::string, std::string>& exfields = std::map<std::string, std::string>());

    inline void setConnectionData(const std::string& host, short port)
    {
        m_host = host;
        m_port = std::to_string(port);
    }

    void setProxyData(const std::string& mode,
                      const std::string& host = "",
                      const std::string& port = "");

    inline void reset_requests(){while(m_req_resp_list.size() > 1){m_req_resp_list.pop_front();}}
    void cleanup_requests(uint64_t interval);

    static void req_to_log(const http::request<http::string_body>& req);
    static void resp_to_log(const http::response<http::string_body>& resp, bool body = false);
    static inline void req_resp_to_log(const http::request<http::string_body>& req, const http::response<http::string_body>& resp, bool body = false)
    {
        req_to_log(req);
        resp_to_log(resp, body);
    }

private:
    void on_resolve_host(boost::system::error_code ec, tcp::resolver::results_type results);
    void on_connect_host(boost::system::error_code ec);
    void on_resolve_proxy(boost::system::error_code ec, tcp::resolver::results_type results);
    void on_connect_proxy(boost::system::error_code ec);
    void on_request_proxy(boost::system::error_code ec, std::size_t bytes_transferred);
    void on_replay_proxy(boost::system::error_code ec, std::size_t bytes_transferred);
    void on_handshake(boost::system::error_code ec);
    void do_write();
    void on_write(const boost::system::error_code, std::size_t bytes_transferred);
    void on_read(const boost::system::error_code, std::size_t bytes_transferred);
    void on_reconnect();
    void on_shutdown(beast::error_code ec);

    int hmac_sha256(const std::string& key, std::string& data, std::string& result);

    static void result(char const* name, beast::error_code ec, char const* what)
    {
        if(!ec)
            mlogger::info("%s: %s - Ok", name, what);
        else
            mlogger::error("%s: %s - Err %s", name, what, ec.message().c_str());
    }

private:
    std::string m_name;
    asio::io_context& m_ioc;

    tcp::resolver m_resolver;
    beast::flat_buffer m_buffer;
    beast::ssl_stream<beast::tcp_stream> m_stream;    
    boost::asio::strand<boost::asio::executor> m_strand;

    http_req_resp_list m_req_resp_list;
    boost::mutex m_req_mutex;

    socks4::request socks_request{socks4::request::connect};
    socks4::reply socks_reply;

    std::string m_host;
    std::string m_port;

    ProxyMode m_proxy_mode;
    std::string m_proxy_host;
    std::string m_proxy_port;

    callback_response m_msg_handler;
    callback_connect m_connect_handler;

    int m_long_poll_timeout;    
    bool m_ready{false};
    bool m_write{false};

    boost::mutex m_wr_mutex;
    asio::deadline_timer m_read_timer;
};

#endif // HTTPS_CLIENT_H

