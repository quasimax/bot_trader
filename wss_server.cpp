#include "wss_server.h"

#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/algorithm/string.hpp>

#include <regex>


void op_result(const char* name, const char* op, boost::system::error_code ec)
{
    if(!ec)
        mlogger::info("%s: %s - Ok", name, op);
    else
        mlogger::error("%s: %s - %s", name, op, ec.message().c_str());
}

//--------------------------------------------------------------------------------------------------

session::session(tcp::socket socket,
                 ssl::context& ctx,
                 swi_callbacs_t& callbacks) : m_socket(std::move(socket)),
                                              m_ws(m_socket, ctx),
                                              m_strand(boost::asio::make_strand(m_ws.get_executor())),
                                              m_ping_timer(m_ws.get_executor(), boost::posix_time::seconds(1)),
                                              m_callbacks(callbacks)
{
}

session::~session()
{
    m_ping_timer.cancel();
    close();
}

void session::close()
{
    beast::error_code ec;
    m_ws.close(websocket::close_code::normal, ec);
    op_result("Web client session", "close", ec);
}

void session::on_ping_timer(boost::system::error_code ec)
{
    if(ec && ec != boost::asio::error::operation_aborted)
    {
        op_result("Web client session", "timer", ec);
        return;
    }

    if(m_ws.is_open() && 0 < --m_ping_counter)
    {
        m_ws.async_ping({},  boost::asio::bind_executor(m_strand, std::bind(&session::on_ping,
                                                                            shared_from_this(),
                                                                            std::placeholders::_1)));


    }
    else
    {
        m_ws.next_layer().next_layer().shutdown(tcp::socket::shutdown_both, ec);
        m_ws.next_layer().next_layer().close(ec);
    }
}

void session::on_ping(boost::system::error_code ec)
{
    if(ec)
    {
        op_result("Web client session", "ping", ec);
        return;
    }

    m_ping_timer.expires_from_now(boost::posix_time::seconds(5));
    m_ping_timer.async_wait(boost::asio::bind_executor(m_strand, std::bind(&session::on_ping_timer,
                                                                           shared_from_this(),
                                                                           std::placeholders::_1)));

}

void session::on_control_callback(websocket::frame_type kind, boost::beast::string_view payload)
{
        boost::ignore_unused(kind, payload);
        m_ping_counter = PING_ATTEMPT;
}



void session::run()
{
    //m_ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));

    m_ws.next_layer().async_handshake(ssl::stream_base::server,
                                      boost::asio::bind_executor(m_strand, std::bind(&session::on_handshake,
                                                                                     shared_from_this(),
                                                                                     std::placeholders::_1)));
}

void session::on_handshake(boost::system::error_code ec)
{
    op_result("Web client session", "handshake", ec);

    if(ec)
        return;

    m_ws.async_accept(boost::asio::bind_executor(m_strand, std::bind(&session::on_accept,
                                                                     shared_from_this(),
                                                                     std::placeholders::_1)));
}

void session::on_accept(beast::error_code ec)
{
    op_result("Web client session", "accept", ec);

    if(ec)
        return;

    m_ws.control_callback(boost::asio::bind_executor(m_strand, std::bind(&session::on_control_callback,
                                                                        this,
                                                                        std::placeholders::_1,
                                                                        std::placeholders::_2)));


    m_ping_counter = PING_ATTEMPT + 1;
    on_ping_timer(make_error_code(boost::system::errc::success));
    do_read();
}

void session::do_read()
{
    m_ws.async_read(m_r_buffer, boost::asio::bind_executor(m_strand, std::bind(&session::on_read,
                                                                             shared_from_this(),
                                                                             std::placeholders::_1,
                                                                             std::placeholders::_2)));
}

void session::on_read(beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

     if(ec)
     {
         op_result("Web client session", "read", ec);
         m_ping_timer.cancel();
         m_ws.async_close(websocket::close_code::abnormal, boost::asio::bind_executor(m_strand, std::bind(&session::on_close,
                                                                                                          shared_from_this(),
                                                                                                          std::placeholders::_1)));

         return;
     }

     m_callbacks.receive_handler(shared_from_this(), beast::buffers_to_string(m_r_buffer.data()));
     m_r_buffer.consume(m_r_buffer.size());
     do_read();
}

void session::send(const std::string& data)
{
    asio::post(m_socket.get_executor(),
               boost::asio::bind_executor(m_strand,
                                          std::bind(&session::on_send,
                                                    shared_from_this(),
                                                    data)));
}

void session::on_send(const std::string& ss)
{
    beast::multi_buffer w_buffer;
    boost::beast::ostream(w_buffer) << ss;
    m_w_queue.push_back(w_buffer);

    if(!m_ws.is_open())
        return;

    if(m_w_queue.size() > 1)
        return;

    m_ws.async_write(m_w_queue.front().data(),
                     boost::asio::bind_executor(m_strand,
                                                std::bind(&session::on_write,
                                                          shared_from_this(),
                                                          std::placeholders::_1,
                                                          std::placeholders::_2)));
}

void session::on_write(beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if(ec)
    {
        op_result("Web client session", "write", ec);
        m_ping_timer.cancel();
        m_ws.async_close(websocket::close_code::abnormal,
                         boost::asio::bind_executor(m_strand,
                                                    std::bind(&session::on_close,
                                                              shared_from_this(),
                                                              std::placeholders::_1)));

    }

    m_w_queue.erase(m_w_queue.begin());

    if(m_w_queue.empty())
        return;

    m_ws.async_write(m_w_queue.front().data(),
                     boost::asio::bind_executor(m_strand,
                                                std::bind(&session::on_write,
                                                          shared_from_this(),
                                                          std::placeholders::_1,
                                                          std::placeholders::_2)));
}

void session::on_close(beast::error_code ec)
{
    op_result("Web client session", "close", ec);
    m_callbacks.disconnect_handler(shared_from_this(), ec);    
}

listener::listener(boost::asio::io_context& ioc, ssl::context& ctx, tcp::endpoint endpoint, swi_callbacs_t& callbacks) : m_ctx(ctx),
                                                                                                                        m_acceptor(ioc),
                                                                                                                        m_socket(ioc),
                                                                                                                        m_callbacks(callbacks)
{
    boost::system::error_code ec;

//    boost::asio::socket_base::reuse_address option(true);
//    m_acceptor.set_option(option);

    m_acceptor.open(endpoint.protocol(), ec);
    op_result("Web listner", "open", ec);
    if(ec)
        return;

    m_acceptor.set_option(net::socket_base::reuse_address(true), ec);
    op_result("Web listner", "set option", ec);
    if(ec)
        return;

    m_acceptor.bind(endpoint, ec);
    op_result("Web listner", "bind", ec);
    if(ec)
        return;

    m_acceptor.listen(MAX_CONNECT, ec);
    op_result("Web listner", "listen", ec);
    if(ec)
        return;

    m_ready = true;
}



//------------------------------------------------------------------------------

void listener::do_accept()
{
    m_acceptor.async_accept(m_socket, std::bind(&listener::on_accept,
                                                shared_from_this(),
                                                std::placeholders::_1));
}

void listener::on_accept(beast::error_code ec)
{
    op_result("Web listner", "open", ec);
    if(ec)
        return;

    auto sess = std::make_shared<session>(std::move(m_socket), m_ctx, m_callbacks);
    m_callbacks.connect_handler(sess, ec);
    sess->run();

    // Accept another connection
    do_accept();
};

//------------------------------------------------------------------------------


WSS_Server::WSS_Server(const std::string& name,
                       boost::asio::io_context& ioc,
                       uinterface& intf,
                       std::vector<std::string>& symbols_list,
                       callback_event event_handler) : m_ioc(ioc),
                                                       m_interface(intf),
                                                       m_symbols_list(symbols_list),
                                                       m_name(name),
                                                       m_event_handler(event_handler)

{
    m_callbacks.connect_handler = std::bind(&WSS_Server::on_connect, this, std::placeholders::_1, std::placeholders::_2);
    m_callbacks.disconnect_handler = std::bind(&WSS_Server::on_disconnect, this, std::placeholders::_1, std::placeholders::_2);
    m_callbacks.receive_handler = std::bind(&WSS_Server::on_receive, this, std::placeholders::_1, std::placeholders::_2);
}

WSS_Server::~WSS_Server()
{
    while(m_client_map.size())
        m_client_map.erase(m_client_map.begin());
}

void WSS_Server::set_certificate_chain_file(const std::string& fcert)
{
    try
    {
        m_ctx.use_certificate_chain_file(fcert);
    }
    catch(std::exception& e)
    {
        mlogger::error("%s.%s: %s", m_name.c_str(), __func__, e.what());
    }
}

void WSS_Server::set_private_key_file(const std::string& fkey)
{
    try
    {
        m_ctx.use_private_key_file(fkey, boost::asio::ssl::context::pem);
    }
    catch(std::exception& e)
    {
        mlogger::error("%s.%s: %s", m_name.c_str(), __func__, e.what());
    }
}


void WSS_Server::run()
{    
    auto const address = asio::ip::make_address("0.0.0.0");
    std::make_shared<listener>(m_ioc, m_ctx, tcp::endpoint{address, m_port}, m_callbacks)->run();
}


void WSS_Server::send(const pt::ptree& pt, int client)
{
    std::ostringstream oss;
    pt::write_json(oss, pt);
    send(oss.str(), client);
}

void WSS_Server::send(const std::string& data, int client_id)
{
    if(-2 == client_id)
    {
        for(const auto& it : m_client_map)
        {
            if(it.second->id() >= 0)
                it.second->send(encode64(data));
        }
    }
    else
    {
        auto it = m_client_map.find(client_id);
        if(m_client_map.end() != it)
        {
            if(it->second->id() >= 0)
                it->second->send(encode64(data));
        }
        else
        {
            mlogger::error("%s.%s: Клиент с идентификатором %d не найден в списке", m_name.c_str(), __func__, client_id);
            return;
        }
    }
}

void WSS_Server::change_client_key(const std::shared_ptr<session> client, int new_key)
{
    for(auto it = m_client_map.begin(); it != m_client_map.end(); ++it)
    {
        if(it->second == client)
        {
            m_client_map.erase(it);
            if(new_key >= 0)                
            {
                mlogger::error("%s.%s: Изменен идентификатор клиента на %d", m_name.c_str(), __func__, new_key);
                m_client_map.emplace(new_key, client);
            }
            break;
        }
    }
}

void WSS_Server::on_connect(const std::shared_ptr<session> client, boost::system::error_code& ec)
{
    op_result(m_name.c_str(), "client connect", ec);
    m_client_map.emplace(-1, client);
}

void WSS_Server::on_disconnect(const std::shared_ptr<session> client, boost::system::error_code& ec)
{
    op_result(m_name.c_str(), "client disconnect", ec);
    mlogger::error("%s.%s: Разрыв соединения с клиентом id %d", m_name.c_str(), __func__, client->id());


    auto it = m_client_map.find(client->id());

    if(m_client_map.end() != it)
    {
        mlogger::error("%s.%s: Удаление клиента id %d из списка", m_name.c_str(), __func__, client->id());
        m_client_map.erase(it);
    }
    else
    {
        mlogger::error("%s.%s: Клиент id %d не найден в списке", m_name.c_str(), __func__, client->id());
    }
}

void WSS_Server::on_receive(const std::shared_ptr<session> client, const std::string& msg)
{
    pt::ptree pt;
    std::istringstream bstr(decode64(msg));

    try
    {
        pt::read_json(bstr, pt);
    }
    catch (std::exception const& e)
    {
        mlogger::error("%s.%s: Ошибка формата данных от клиента id %d: %s\n%s", m_name.c_str(),
                                                                                __func__,
                                                                                client->id(),
                                                                                e.what(),
                                                                                msg.c_str());
        std::string msg = std::string("parse json error: ") + e.what();
        error_message(client, 3);
        return;
    }

    mlogger::info("%s.%s: Получены данные от клиента id %d:\n%s", m_name.c_str(),
                                                                  __func__,
                                                                  client->id(),
                                                                  bstr.str().c_str());

    std::string cmd = pt.get<std::string>("c");

    if("authUser" == cmd)
    {
        pt::ptree apt;
        std::string password = pt.get<std::string>("p", "");

        apt.put<std::string>("e", "authUser");

        auto range = m_client_map.equal_range(-1);
        auto it = range.first;
        for(; it != range.second; ++it)
        {
            if(it->second == client)
                break;
        }

        if(range.second == it)
        {
            mlogger::warn("%s.%s: Попытка повторной авторизации пользователя", m_name.c_str(), __func__);
            return;
        }

        if(password == m_admin_pssword)
        {
            apt.put<std::string>("r", "admin");

            if(IntfTlg == m_interface)
            {
                apt.put<int>("с", 2);                
                apt.put<std::string>("m", "Пароль принят. Управление от Telegram");
                mlogger::warn("%s.%s: Авторизация пользователя admin, управление от Telegram", m_name.c_str(), __func__);
                goto as_user;
            }


            if(m_client_map.find(0) == m_client_map.end())
            {
                client->set_id(0);
                change_client_key(client, 0);
                apt.put<int>("с", 0);
                apt.put<std::string>("m", "Пароль принят");
                mlogger::warn("%s.%s: Авторизация пользователя admin - Пароль принят", m_name.c_str(), __func__);
            }
            else
            {
                apt.put<int>("c", 1);
                apt.put<std::string>("r", "admin");
                apt.put<std::string>("m", "Пользователь уже авторизован");
                mlogger::warn("%s.%s: Авторизация пользователя admin - Уже занят", m_name.c_str(), __func__);
            }
        }
        else if(password == m_user_pssword)
        {
as_user:    client->set_id(++m_curr_id);
            change_client_key(client, m_curr_id);
            apt.put<int>("c", 0);
            apt.put<std::string>("r", "user");
            apt.put<std::string>("m", "Пароль принят. Управление блокировано");
            mlogger::warn("%s.%s: Авторизация пользователя user (id %d) - Пароль принят", m_name.c_str(), __func__, client->id());
        }
        else
        {
            apt.put<int>("c", 0);
            apt.put<std::string>("r", "");
            apt.put<std::string>("m", "Password fail");
            mlogger::warn("%s.%s: Авторизация пользователя - Неправильный пароль", m_name.c_str(), __func__, client->id());
        }

        std::ostringstream oss;
        pt::write_json(oss, apt);
        client->send(encode64(oss.str()));
        return;
    }

    if(client->id() < 0)    // проверка авторизации
    {
        mlogger::warn("%s.%s: Команда от неавторизованного пользователя - Блокировано", m_name.c_str(), __func__);
        return;
    }

    command_decoder(client, cmd, pt);
}

void WSS_Server::command_decoder(const std::shared_ptr<session> client, const std::string& cmd, const pt::ptree& pt)
{
    mlogger::info("%s-%s: Обработка команды от клиента id %d: %s", m_name.c_str(), __func__, client->id(), cmd);

    if("runTrade" == cmd)
    {
        if(0 != client->id())
            error_message(client, 2);
        else
        {
            std::ostringstream oss;
            pt::write_json(oss, pt);
            m_event_handler(Events::EVENT_WEB_TRADE_RUN, boost::variant<std::string>(oss.str()));
        }
    }
    else if("ordersList" == cmd)
    {
        if("open" == pt.get<std::string>("m"))
            m_event_handler(Events::EVENT_ORD_REQ_LIST, boost::variant<std::string>(pt.get<std::string>("s")));
        else if("all" == pt.get<std::string>("m"))
            m_event_handler(Events::EVENT_ORD_REQ_LIST, boost::variant<std::string>(""));
    }
    else if("outboundAccountInfo" == cmd)
    {
        m_event_handler(Events::EVENT_TLG_GET_ACCOUNT, boost::variant<char>('\0'));
    }
    else if("ordersDel" == cmd)
    {
        if(0 != client->id())
        {
            error_message(client, 2);
            return;
        }

        bnc_order_t order;
        try
        {
            order.newClientOrderId = pt.get<std::string>("n");
            order.symbol = pt.get<std::string>("s", "") + "USDT";
        }
        catch(std::exception& e)
        {
            mlogger::error("%s: DELETE order parser - %s", m_name.c_str(), e.what());
            return;
        }

        m_event_handler(Events::EVENT_ORD_DELETE, boost::variant<bnc_order_t>(order));
    }
    else if("tradeData" == cmd)
    {
        if(0 != client->id())
        {
            std::string oper = pt.get<std::string>("a");

            if("get" != oper)
            {
                error_message(client, 1);
                return;
            }
        }

        std::ostringstream oss;
        pt::write_json(oss, pt);

        m_event_handler(Events::EVENT_WEB_SET_TRADE_DATA, boost::variant<std::string>(oss.str()));
    }
    else if("tradeState" == cmd)
    {
        m_event_handler(Events::EVENT_WEB_REQ_TRADE_STATE, boost::variant<std::string>(pt.get<std::string>("s")));
    }
    else if("priceSymbol" == cmd)
    {
        m_event_handler(Events::EVENT_SCR_GET_PRICE, boost::variant<std::string>(pt.get<std::string>("s")));
    }
    else if("tradePos" == cmd)
    {
        m_event_handler(Events::EVENT_WEB_REQ_TRADE_POS, boost::variant<std::string>(pt.get<std::string>("s")));
    }
    else if("getSymbols" == cmd)
    {
        pt::ptree spt, lpt;
        spt.put<std::string>("e", "getSymbols");

        for(auto sit : m_symbols_list)
            lpt.push_back(pt::ptree::value_type("", sit));

        spt.add_child("s", lpt);

        std::ostringstream oss;
        pt::write_json(oss, spt);
        client->send(encode64(oss.str()));
        client->set_ready_for_price(true);
    }
    else if("getFilters" == cmd)
    {
        m_event_handler(Events::EVENT_WEB_GET_FILTERS, boost::variant<char>('\0'));
    }
    else
    {
        mlogger::error("%s.%s: Команда от клиента не опознана", m_name.c_str(), __func__);
        error_message(client, 3);
    }
}

void WSS_Server::error_message(const std::shared_ptr<session> client, int ecode)
{
    pt::ptree ept;

    ept.put<std::string>("e", "errorMsg");
    ept.put<int>("c", ecode);

    switch(ecode)
    {
        case 1:
            ept.put<std::string>("m", "Внутренняя ошибка");
            break;

        case 2:
            ept.put<std::string>("m", "Доступ запрещен");
            break;

        case 3:
            ept.put<std::string>("m", "Ошибка формата данных");
            break;

        default:
            ept.put<std::string>("m", "Неизвестная ошибка");
    }

    std::ostringstream oss;
    pt::write_json(oss, ept);
    client->send(encode64(oss.str()));
}

void WSS_Server::on_price(const std::string& data)
{
    pt::ptree ipt, opt;
    std::istringstream bstr(boost::get<std::string>(data));
    pt::read_json(bstr, ipt);
    opt.put<std::string>("e", "priceSymbol");
    std::string symbol = ipt.get<std::string>("symbol");
    //std::regex_replace(symbol, std::regex("USDT"), "");
    opt.put<std::string>("s", symbol);
    opt.put<double>("p", ipt.get<double>("price"));

    for(const auto& cit : m_client_map)
    {
        if(cit.second->id() < 0)
            continue;

        if(!cit.second->ready_for_price())
            continue;

        std::ostringstream oss;
        pt::write_json(oss, opt);
        cit.second->send(encode64(oss.str()));
    }
}

void WSS_Server::on_binance_error(const std::string& data)
{
    pt::ptree ipt, opt;
    std::istringstream bstr(boost::get<std::string>(data));
    pt::read_json(bstr, ipt);
    opt.put<std::string>("e", "binanceError");
    opt.put<std::string>("c", ipt.get<std::string>("code"));
    opt.put<std::string>("m", ipt.get<std::string>("msg"));
    send(opt);
}

void WSS_Server::on_account(const std::string& data)
{
    pt::ptree ipt, opt;
    std::istringstream bstr(boost::get<std::string>(data));
    pt::read_json(bstr, ipt);

    opt.put<std::string>("e", "outboundAccountInfo");
    opt.put<std::string>("m", ipt.get<std::string>("makerCommission"));
    opt.put<std::string>("t", ipt.get<std::string>("takerCommission"));

    auto blnc = ipt.get_child("balances");
    pt::ptree balances;

    for(auto it = blnc.begin(); it != blnc.end(); ++it)
    {
        pt::ptree pt_symbol;
        std::string symbol = it->second.get<std::string>("asset");

        if(m_symbols_list.end() == std::find(m_symbols_list.begin(), m_symbols_list.end(), symbol)
        && symbol != "USDT")
            continue;

        pt_symbol.put("a", symbol);
        pt_symbol.put("f", it->second.get<std::string>("free"));
        pt_symbol.put("l", it->second.get<std::string>("locked"));
        balances.push_back(pt::ptree::value_type("", pt_symbol));
    }

    opt.add_child("B", balances);
    send(opt);
}

void WSS_Server::on_orders_list(const std::string& data)
{
    pt::ptree ipt, opt;
    std::istringstream bstr(boost::get<std::string>(data));
    pt::read_json(bstr, ipt);

    opt.put<std::string>("e", "ordersList");
    opt.add_child("orders", ipt);

    send(opt);
}

void WSS_Server::on_trade_steps(const std::string& symbol, const std::vector<std::string>& pos_list)
{
    pt::ptree opt, ppt;
    opt.put<std::string>("e", "tradePos");
    opt.put<std::string>("s", symbol);

    for(auto it : pos_list)
        ppt.push_back(pt::ptree::value_type("", it));

    opt.add_child("p", ppt);
    send(opt);
}

std::string WSS_Server::decode64(const std::string &val)
{
    using namespace boost::archive::iterators;
    using It = transform_width<binary_from_base64<std::string::const_iterator>, 8, 6>;
    return boost::algorithm::trim_right_copy_if(std::string(It(std::begin(val)), It(std::end(val))), [](char c){return c == '\0';});
}

std::string WSS_Server::encode64(const std::string &val)
{
    using namespace boost::archive::iterators;
    using It = base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>;
    auto tmp = std::string(It(std::begin(val)), It(std::end(val)));
    return tmp.append((3 - val.size() % 3) % 3, '=');
}
