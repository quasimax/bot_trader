#include "https_client.h"
#include "utils.h"

HTTPS_Client::HTTPS_Client(const std::string& name,
                           asio::io_context& ioc,
                           ssl::context& ctx,
                           callback_connect chandler,
                           callback_response mhandler)

             : m_name(name),
               m_ioc(ioc),
               m_resolver(ioc),
               m_stream(ioc, ctx),
               m_strand(boost::asio::make_strand(m_stream.get_executor())),
               m_msg_handler(mhandler),
               m_connect_handler(chandler),
               m_read_timer(ioc, boost::posix_time::seconds(1))

{

}

HTTPS_Client::~HTTPS_Client()
{
    stop();
}

void HTTPS_Client::stop()
{
    beast::get_lowest_layer(m_stream).close();
    mlogger::info("%s: STOPPED", m_name.c_str());
}

void HTTPS_Client::connect()
{
    switch(m_proxy_mode)
    {
        case ProxyMode::socks4a:
            {
                mlogger::info("%s: соединение с PROXY(socks4a)..", m_name.c_str());
                tcp::resolver::query socks_query(m_proxy_host, m_proxy_port);
                m_resolver.async_resolve(m_proxy_host,
                                         m_proxy_port,
                                         boost::asio::bind_executor(m_strand,
                                                                    std::bind(&HTTPS_Client::on_resolve_proxy,
                                                                              this,
                                                                              std::placeholders::_1,
                                                                              std::placeholders::_2)));
            }
            break;

        case ProxyMode::noproxy:
            mlogger::info("%s: соединение с HOST..", m_name.c_str());
            m_resolver.async_resolve(m_host,
                                     m_port,
                                     boost::asio::bind_executor(m_strand,
                                                                std::bind(&HTTPS_Client::on_resolve_host,
                                                                          this,
                                                                          std::placeholders::_1,
                                                                          std::placeholders::_2)));
            break;

        default:
            return;
    }
}

void HTTPS_Client::on_resolve_host(boost::system::error_code ec, tcp::resolver::results_type results)
{
    result(m_name.c_str(), ec, "resolve");

    if(ec)
    {
        m_connect_handler(!ec.value());
        m_ioc.post(boost::asio::bind_executor(m_strand,
                                              boost::bind(&HTTPS_Client::on_reconnect, this)));
        return;
    }

    beast::get_lowest_layer(m_stream).async_connect(results,
                                                    boost::asio::bind_executor(m_strand,
                                                                               std::bind(&HTTPS_Client::on_connect_host,
                                                                                         this,
                                                                                         std::placeholders::_1)));
}

void HTTPS_Client::on_connect_host(boost::system::error_code ec)
{
    result(m_name.c_str(), ec, "connect to host");

    if(ec)
    {
        m_connect_handler(!ec.value());
        m_ioc.post(boost::asio::bind_executor(m_strand,
                                              boost::bind(&HTTPS_Client::on_reconnect,
                                                          this)));
        return;
    }

    m_stream.async_handshake(ssl::stream_base::client,
                             boost::asio::bind_executor(m_strand,
                                                        std::bind(&HTTPS_Client::on_handshake,
                                                                 this,
                                                                 std::placeholders::_1)));
}


void HTTPS_Client::on_resolve_proxy(boost::system::error_code ec, tcp::resolver::results_type results)
{
    result(m_name.c_str(), ec, "socks4a proxy resolve");

    if(ec)
    {
        m_connect_handler(!ec.value());
        m_ioc.post(boost::asio::bind_executor(m_strand,
                                              boost::bind(&HTTPS_Client::on_reconnect,
                                                          this)));
        return;
    }

    beast::get_lowest_layer(m_stream).async_connect(results,
                                                    boost::asio::bind_executor(m_strand,
                                                                               std::bind(&HTTPS_Client::on_connect_proxy,
                                                                               this,
                                                                               std::placeholders::_1)));
}

void HTTPS_Client::on_connect_proxy(boost::system::error_code ec)
{
    result(m_name.c_str(), ec, "socks4a proxy connect");

    if(ec)
    {
        m_connect_handler(!ec.value());
        m_ioc.post(boost::asio::bind_executor(m_strand,
                                              boost::bind(&HTTPS_Client::on_reconnect,
                                                          this)));
        return;
    }

    asio::ip::address ip_address = asio::ip::address::from_string("0.0.0.1");
    asio::ip::tcp::endpoint ep(ip_address, 443);
    socks4::request socks_request(socks4::request::connect, ep, m_host);

    asio::async_write(beast::get_lowest_layer(m_stream),
                      socks_request.buffers_ep(ep),
                      boost::asio::bind_executor(m_strand, std::bind(&HTTPS_Client::on_request_proxy,
                                                                     this,
                                                                     std::placeholders::_1,
                                                                     std::placeholders::_2)));
}

void HTTPS_Client::on_request_proxy(boost::system::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);
    result(m_name.c_str(), ec, "socks4a proxy request");

    if(ec)
    {
        m_connect_handler(!ec.value());
        m_ioc.post(boost::asio::bind_executor(m_strand,
                                              boost::bind(&HTTPS_Client::on_reconnect,
                                                          this)));
        return;
    }

    boost::asio::async_read(beast::get_lowest_layer(m_stream),
                            socks_reply.buffers(), boost::asio::bind_executor(m_strand,
                                                                              std::bind(&HTTPS_Client::on_replay_proxy,
                                                                                        this,
                                                                                        std::placeholders::_1,
                                                                                        std::placeholders::_2)));
}

void HTTPS_Client::on_replay_proxy(boost::system::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);
    result(m_name.c_str(), ec, "socks4a proxy replay");

    if(ec)
    {
        m_connect_handler(!ec.value());
        m_ioc.post(boost::asio::bind_executor(m_strand,
                                              boost::bind(&HTTPS_Client::on_reconnect,
                                                          this)));
        return;
    }

    m_stream.async_handshake(ssl::stream_base::client,
                             boost::asio::bind_executor(m_strand,
                                                        std::bind(&HTTPS_Client::on_handshake,
                                                                 this,
                                                                 std::placeholders::_1)));
}

void HTTPS_Client::on_handshake(boost::system::error_code ec)
{
    result(m_name.c_str(), ec, "SSL handshake");
    m_connect_handler(!ec.value());

    if(ec)
    {
        m_ioc.post(boost::asio::bind_executor(m_strand,
                                              boost::bind(&HTTPS_Client::on_reconnect,
                                                          this)));
        return;
    }

    m_ready = true;

    boost::lock_guard<boost::mutex> lock(m_wr_mutex);

    if(!m_req_resp_list.empty())
    {
        do_write();
    }
}

int HTTPS_Client::hmac_sha256(const std::string& key, std::string& data, std::string& result)
{
    unsigned int size;
    unsigned char* pbuf;

    pbuf = HMAC(EVP_sha256(),
                key.data(), key.size(),
                reinterpret_cast<const unsigned char*>(data.data()), data.size(),
                NULL, &size);

    std::vector<unsigned char> v(pbuf, pbuf + size);
    boost::algorithm::hex(v.begin(), v.end(), back_inserter(result));

    return size;
}

void HTTPS_Client::request(http::verb method,
                          const std::string& target,
                          ContentType content_type,
                          const pt::ptree& payload,
                          const std::string& secret_key,
                          const std::map<std::string, std::string>& exfields)
{
    http_req_resp_t req_resp;

    req_resp.request.method(method);
    req_resp.request.version(11);
    req_resp.request.set(http::field::host, m_host);
    req_resp.request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req_resp.request.set(http::field::accept, "*/*");
    req_resp.request.keep_alive(true);

    for(auto&& ev : exfields)
    {
        req_resp.request.insert(ev.first, ev.second);
    }

    switch(content_type)
    {
        case ContentType::json:
            {
                req_resp.request.target(target);
                req_resp.request.set(http::field::content_type, "application/json; charset=utf-8");
                std::ostringstream os;
                pt::json_parser::write_json(os, payload, false);                
                std::regex reg("\\\"(true)\\\"");
                //std::regex reg("\\\"([0-9]+\\.{0,1}[0-9]*)(true)(false)\\\"");
                req_resp.request.body() = std::regex_replace(os.str(), reg, "$1");
                req_resp.request.prepare_payload();

            }
            break;

        case ContentType::text_palin:
            {
                req_resp.request.target(target);
                req_resp.request.set(http::field::content_type, "text/plain");

                for(auto&& rv : payload)
                    req_resp.request.insert(rv.first, rv.second.get_value<std::string>());

                if(!secret_key.empty())
                {
                    std::string signature;
                    std::string spl = req_resp.request.body();
                    hmac_sha256(secret_key, spl, signature);
                    req_resp.request.insert("signature", signature);
                }
            }
            break;

        case ContentType::urlencoded:
            {
                req_resp.request.target(target);
                req_resp.request.set(http::field::content_type, "application/x-www-form-urlencoded");

                std::ostringstream os;
                std::string params;

                for(auto&& rv : payload)
                {
                    params.append(rv.first + "=" + rv.second.get_value<std::string>());
                    params.append("&");
                }

                params.pop_back();
                os << params;

                if(!secret_key.empty())
                {
                    std::string signature;
                    std::string spl = os.str();
                    hmac_sha256(secret_key, spl, signature);
                    os << "&signature=" << signature;
                }

                req_resp.request.body() = os.str();
                req_resp.request.prepare_payload();

            }
            break;

        case ContentType::nocontent:
            {
                std::string params;

                for(auto&& rv : payload)
                {
                    params.append(rv.first + "=" + rv.second.get_value<std::string>());
                    params.append("&");
                }

                if(!secret_key.empty())
                {
                    std::string signature;
                    hmac_sha256(secret_key, params, signature);
                    params.append("&signature=" + signature);
                }
                else if(!params.empty())
                {
                    params.pop_back();  // удаление хвостового '&'
                }

                if(!params.empty())
                    params.insert(0, "?");

                req_resp.request.target(target + params);
            }
            break;

        default:
            return;
    }

    boost::lock_guard<boost::mutex> lock(m_wr_mutex);
    req_resp.push_time = utils::curr_time_milliseconds();
    req_resp.send_time = 0;
    m_req_resp_list.push_back(req_resp);

    if(!m_ready)
        return;    

    if(m_req_resp_list.size() > 1)
    {        
        return;
    }

    do_write();
}

void HTTPS_Client::do_write()
{
//    beast::get_lowest_layer(m_stream).expires_after(std::chrono::seconds(40));

    m_req_resp_list.front().send_time =  std::time(nullptr);

    http::async_write(m_stream,
                      m_req_resp_list.front().request,
                      boost::asio::bind_executor(m_strand, std::bind(&HTTPS_Client::on_write,
                                                                     this,
                                                                     std::placeholders::_1,
                                                                     std::placeholders::_2)));
}

void HTTPS_Client::on_write(const boost::system::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if(ec)
    {        
        m_req_resp_list.front().send_time = 0;
        m_ioc.post(boost::asio::bind_executor(m_strand,
                                              boost::bind(&HTTPS_Client::on_reconnect,
                                                          this)));
        return result(m_name.c_str(), ec, "HTTPS write");
    }    

/*
    m_read_timer.expires_from_now(boost::posix_time::seconds(5));
    m_read_timer.async_wait(boost::asio::bind_executor(m_strand, [&](const boost::system::error_code ec)
                                                                 {

                                                                 }));

                                                                           std::bind(&HTTPS_Client::on_read,
                                                                           this,
                                                                           asio::placeholders::error,
                                                                           0)));
*/

    http::async_read(m_stream,
                     m_buffer,
                     m_req_resp_list.front().response,
                     boost::asio::bind_executor(m_strand, std::bind(&HTTPS_Client::on_read,
                                                                    this,
                                                                    std::placeholders::_1,
                                                                    std::placeholders::_2)));
}

void HTTPS_Client::on_read(const boost::system::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);        

    if(ec)
    {
        m_req_resp_list.front().send_time = 0;
        m_ioc.post(boost::bind(&HTTPS_Client::on_reconnect, this));
        return result(m_name.c_str(), ec, "HTTPS read");
    }

    m_msg_handler(m_req_resp_list.front().request,
                  m_req_resp_list.front().response);

    boost::lock_guard<boost::mutex> lock(m_wr_mutex);
    m_req_resp_list.pop_front();

    if(m_req_resp_list.empty())
        return;

    do_write();
}

void HTTPS_Client::setProxyData(const std::string& mode,
                                const std::string& host,
                                const std::string& port)
{
    if("socks4a" == mode)
        m_proxy_mode = ProxyMode::socks4a;
    else if("http" == mode)
        m_proxy_mode = ProxyMode::http;
    else
        m_proxy_mode = ProxyMode::noproxy;

    m_proxy_host = host;
    m_proxy_port = port;
}

void HTTPS_Client::on_reconnect()
{
    beast::error_code ec;
    m_ready = false;

    SSL_clear(m_stream.native_handle());
    m_buffer.consume(m_buffer.size());
    beast::get_lowest_layer(m_stream).socket().shutdown(tcp::socket::shutdown_both, ec);
    result(m_name.c_str(), ec, "TCP shutdown");
    beast::get_lowest_layer(m_stream).close();

    m_ioc.post(boost::asio::bind_executor(m_strand,
                                          boost::bind(&HTTPS_Client::connect,
                                                      this)));
}

void HTTPS_Client::on_shutdown(beast::error_code ec)
{
    result(m_name.c_str(), ec, "TCP shutdown");
}

void HTTPS_Client::cleanup_requests(uint64_t interval)
{
    uint64_t curr_time = utils::curr_time_milliseconds();
    int num_del_msg = 0;    

    boost::lock_guard<boost::mutex> lock(m_wr_mutex);

    try
    {
        for(auto it = m_req_resp_list.begin(); it != m_req_resp_list.end(); it++)
        {
            if(0 != it->send_time)
                continue;

            if(curr_time - it->push_time >= interval)
                m_req_resp_list.erase(it--);
            else
                break;
        }
    }
    catch(std::exception& e)
    {
        mlogger::info("%s-%s: Очистка очереди - %s", m_name.c_str(), __func__, e.what());
        return;
    }

    if(0 == num_del_msg)
        return;

    mlogger::info("%s: Очистка очереди сообщений: удалено %d new size %d", m_name.c_str(),
                                                                           num_del_msg,
                                                                           m_req_resp_list.size());
}

void HTTPS_Client::req_to_log(const http::request<http::string_body>& req)
{
    std::stringstream ss;
    std::string fline;
    std::vector<std::string> fields;

    mlogger::info("--- Request --------------------------");

    ss << req;
    std::getline(ss, fline);
    mlogger::info(fline.c_str());

    std::getline(ss, fline);
    mlogger::info(fline.c_str());

    boost::algorithm::split(fields, req.body(), boost::is_any_of("?,&"));
    for(auto it = fields.begin(); it != fields.end(); ++it)
        mlogger::info(it->c_str());

}

void HTTPS_Client::resp_to_log(const http::response<http::string_body>& resp, bool body)
{
    std::stringstream ss;
    std::string fline;

    mlogger::info("--- Response --------------------------");

    ss << "Result: ";
    ss << resp.result_int() << " ";
    ss << resp.reason();

    std::getline(ss, fline);
    mlogger::info(fline.c_str());

    if(body || http::status::ok != resp.result())
    {
        ss.str(std::string());
        ss.clear();
        ss << resp.body();
        mlogger::info(ss.str().c_str());
    }

    mlogger::info("---------------------------------------");
}

