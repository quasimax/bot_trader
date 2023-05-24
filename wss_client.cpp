#include "wss_client.h"

WSS_Client::WSS_Client(const std::string& name,
                       const std::string& stream,
                       asio::io_context& ioc,
                       ssl::context& ctx,
                       callback_connect chandler,
                       callback_receive mhandler) : m_name(name),
                                                    m_stream(stream),
                                                    m_resolver(ioc),
                                                    m_ws(ioc, ctx),
                                                    m_strand(boost::asio::make_strand(m_ws.get_executor())),
                                                    m_data_handler(mhandler),
                                                    m_connect_handler(chandler)
{    
    m_ws.control_callback([&](websocket::frame_type kind, boost::beast::string_view)
                              {
                                if(kind == boost::beast::websocket::frame_type::ping)
                                {
                                    //if(m_connected)
                                    if(state() == connect_state::Connected)
                                        m_ws.pong({});
                                }
                              });

}

WSS_Client::~WSS_Client()
{
    stop();
}

void WSS_Client::stop()
{
    boost::system::error_code ec;
    if(is_open())
        m_ws.close(websocket::close_code::normal, ec);

    m_connect_state = connect_state::Closed;
    mlogger::info("%s: ОСТАНОВЛЕН", m_name.c_str());
}

void WSS_Client::connect()
{
    //m_connecting = true;
    m_connect_state = connect_state::Connecting;
    m_resolver.async_resolve(m_host, m_port, std::bind(&WSS_Client::on_resolve,
                                                       this,
                                                       std::placeholders::_1,
                                                       std::placeholders::_2));
}

void WSS_Client::on_resolve(boost::system::error_code ec, tcp::resolver::results_type results)
{
    result(m_name.c_str(), ec, "Resolve");

    if(ec)
    {
        m_connect_handler(!ec.value());
        m_resolver.async_resolve(m_host, m_port, std::bind(&WSS_Client::on_resolve,
                                                           this,
                                                           std::placeholders::_1,
                                                           std::placeholders::_2));
        return;
    }

    asio::async_connect(m_ws.next_layer().next_layer(), results.begin(), results.end(), std::bind(&WSS_Client::on_connect,
                                                                                                   this,
                                                                                                   std::placeholders::_1));
}

void WSS_Client::on_connect(boost::system::error_code ec)
{
    result(m_name.c_str(), ec, "TCP connect");

    if(ec)
    {
       //m_connecting = false;
       m_connect_state = connect_state::Closed;
       m_connect_handler(!ec.value());       
       return;
    }

    // SSL handshake
    m_ws.next_layer().async_handshake(ssl::stream_base::client, std::bind(&WSS_Client::on_ssl_handshake,
                                                                          this,
                                                                          std::placeholders::_1));
}

void WSS_Client::on_ssl_handshake(boost::system::error_code ec)
{
    result(m_name.c_str(), ec, "SSL handshake");

    if(ec)
    {
        close();        
        m_connect_handler(!ec.value());
        return;
    }    

    m_ws.set_option(websocket::stream_base::timeout::suggested(boost::beast::role_type::client));

    m_ws.async_handshake(m_host,
                         "/stream?streams=" + m_stream,
                         std::bind(&WSS_Client::on_handshake,
                                   this,
                                   std::placeholders::_1));
}

void WSS_Client::on_handshake(boost::system::error_code ec)
{
    result(m_name.c_str(), ec, "WSS handshake");
    m_connect_handler(!ec.value());

    if(ec)
    {
        close();
        return;
    }

    //m_connecting = false;
    //m_connected = true;
    m_connect_state = connect_state::Connected;
    do_read();
}

void WSS_Client::do_read()
{
    m_buffer.clear();
    m_ws.async_read(m_buffer, boost::asio::bind_executor(m_strand, std::bind(&WSS_Client::on_read,
                                                                             this,
                                                                             std::placeholders::_1,
                                                                             std::placeholders::_2)));

}

void WSS_Client::on_read(boost::system::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if(ec)
    {
        result(m_name.c_str(), ec, "WSS read");
        close();
        m_connect_handler(!ec.value());
        return;
    }

    m_data_handler(boost::beast::buffers_to_string(m_buffer.data()));
    do_read();
}

void WSS_Client::close()
{
    //m_connected = false;
    m_connect_state = connect_state::WaitClosed;
    m_ws.async_close(websocket::close_code::abnormal, std::bind(&WSS_Client::on_close,
                                                              this,
                                                              std::placeholders::_1));
}

void WSS_Client::on_close(boost::system::error_code ec)
{
    //m_connecting = false;
    m_connect_state = connect_state::Closed;

    if(ec)
    {
        result(m_name.c_str(), ec, "WSS close");
    }
}
