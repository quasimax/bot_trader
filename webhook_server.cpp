#include "webhook_server.h"


wh_http_client::wh_http_client(boost::asio::io_context& ioc, callback_webhook callback) : m_name("webhook client"),
                                                                                          m_socket(ioc),
                                                                                          m_callback(callback)
{

}

void wh_http_client::start(std::vector<pointer>* clients)
{
    m_clients_list = clients;
    clients->push_back(shared_from_this());
    read_request();
}

void wh_http_client::stop()
{
    m_socket.shutdown(tcp::socket::shutdown_send);

    auto it = find(m_clients_list->begin(), m_clients_list->end(), shared_from_this());
    m_clients_list->erase(it);
}

void wh_http_client::read_request()
{
    http::async_read(m_socket, m_buffer, m_request, [this](boost::beast::error_code ec, std::size_t)
                                                       {
                                                            if(ec)
                                                            {
                                                                mlogger::error("%s Ошибка чтения данных %d: %s",
                                                                               ec.value(),
                                                                               ec.message().c_str());
                                                                stop();
                                                            }
                                                            else
                                                                process_request();
                                                        });
}

void wh_http_client::process_request()
{
    switch (m_request.method())
    {
        case http::verb::post:
            break;

        default:
            send_response(http::status::bad_request);
            return;
    }

    auto content_type = m_request["Content-Type"];

    if(0 != content_type.to_string().find("text/plain"))
    {
        send_response(http::status::unsupported_media_type);
        return;
    }


    m_callback(m_request.body());
    send_response(http::status::ok);
}

void wh_http_client::send_response(http::status status)
{
    m_response.result(status);
    m_response.version(11);
    m_response.set(http::field::server, "Beast");
    m_response.set(http::field::transfer_encoding, "chunked");
    m_response.body() = "";
    m_response.prepare_payload();

    http::async_write(m_socket, m_response, boost::bind(&wh_http_client::stop, this));
}

//=================================================================================

webhook_server::~webhook_server()
{
    m_ioc.stop();
    m_ioc_thread.join();

    while(clients.size())
        clients.front()->stop();

    if(nullptr != m_acceptor)
        delete m_acceptor;
}

void webhook_server::start()
{
    m_acceptor = new tcp::acceptor(m_ioc, ip::tcp::endpoint(/*ip::tcp::v4()*/ip::address_v4::loopback(), m_port));
    start_accept();
    m_ioc.reset();
    m_ioc_thread = boost::thread(boost::bind(&boost::asio::io_context::run, boost::ref(m_ioc)));
}

void webhook_server::start_accept()
{
    wh_http_client::pointer new_client = wh_http_client::create(m_ioc, m_callback);

    m_acceptor->async_accept(new_client->socket(),
                             boost::bind(&webhook_server::on_accept, this, new_client, boost::asio::placeholders::error));

}

void webhook_server::on_accept(wh_http_client::pointer client, const boost::beast::error_code& ec)
{
    if(!ec)
    {
        client->start(&clients);
    }

    start_accept();
}
