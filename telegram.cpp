#include "telegram.h"
#include "html_messages.h"

#include <string>

Telegram::Telegram(const std::string& name,
                   asio::io_context& ioc,
                   ssl::context& ctx,
                   uinterface& intf,
                   std::vector<std::string>& symbols_list,
                   callback_event event_handler) : m_name(name),
                                                   m_ioc(ioc),
                                                   m_ctx(ctx),
                                                   m_interface(intf),
                                                   m_symbols_list(symbols_list),
                                                   m_event_handler(event_handler),
                                                   m_init_timer(ioc, boost::posix_time::seconds(1))
{
    command_map.insert(std::make_pair("/menu", boost::bind(&Telegram::on_command_menu, this, ::_1)));
    command_map.insert(std::make_pair("/account", boost::bind(&Telegram::on_command_account, this, ::_1)));
    command_map.insert(std::make_pair("/price", boost::bind(&Telegram::on_command_price, this, ::_1)));
    command_map.insert(std::make_pair("/indicators", boost::bind(&Telegram::on_command_indicators, this, ::_1)));
    command_map.insert(std::make_pair("/stat", boost::bind(&Telegram::on_command_stat, this, ::_1)));
    command_map.insert(std::make_pair("/cancel", boost::bind(&Telegram::on_command_cancel, this, ::_1)));

    m_init_timer.async_wait(boost::bind(&Telegram::init, this));
}

void Telegram::stop()
{
    client_poll.stop();
    client_req.stop();
}

void Telegram::init()
{
    if(IntfTlg == m_interface)
    {
        command_map.insert(std::make_pair("/order", boost::bind(&Telegram::on_command_order, this, ::_1)));
        command_map.insert(std::make_pair("/script", boost::bind(&Telegram::on_command_script, this, ::_1)));
    }

    client_poll.connect();    
    client_req.connect();

    std::string symbols;
    for(const auto& it : m_symbols_list)
    {
        symbols += it;
        symbols += ", ";
    }
    symbols.resize(symbols.size() - 2);
    send_alarm_message("Бот запущен. Скрипт " + symbols + " остановлен.");
}

void Telegram::request(const std::string& command)
{
    client_req.request(http::verb::get,
                       m_target + "/" + command,
                       HTTPS_Client::ContentType::nocontent);
}

void Telegram::send_alarm_message(const std::string& text)
{
    pt::ptree pt;
    pt.put("chat_id", m_alarm_channel_id);
    pt.put("text", text);

    client_req.request(http::verb::get,
                       m_target + "/sendMessage",
                       HTTPS_Client::ContentType::json,
                       pt);
}


void Telegram::send_message(int chat_id,
                            const std::string& text,
                            const std::string& parse_mode,
                            MessageMode mode,
                            const pt::ptree& keyboard)
{
    pt::ptree pt;
    pt.put("chat_id", chat_id);
    pt.put("text", text);
    if("HTML" == parse_mode || "Markdown" == parse_mode)
        pt.put("parse_mode", parse_mode);

    switch(mode)
    {
        case Keyboard:
            if(keyboard.size())
            {
                pt::ptree kpt;
                kpt.add_child("keyboard", keyboard);
                kpt.put<bool>("resize_keyboard", true);
                kpt.put<bool>("one_time_keyboard", true);

                pt.add_child("reply_markup", kpt);
            }
            else
            {
                pt::ptree kpt;
                kpt.put<bool>("remove_keyboard", true);
                pt.add_child("reply_markup", kpt);
            }
            break;

        case Replay:
            {
                pt::ptree frpt;
                frpt.put<bool>("force_reply", true);
                pt.add_child("reply_markup", frpt);
            }
            break;

        default:
            {
                pt::ptree kpt;
                kpt.put<bool>("remove_keyboard", true);
                pt.add_child("reply_markup", kpt);
            }
            break;
    }

    client_req.request(http::verb::get,
                       m_target + "/sendMessage",
                       HTTPS_Client::ContentType::json,
                       pt);
}

void Telegram::answer_handler(http::request<http::string_body>& req, http::response<http::string_body>& resp)
{
    boost::ignore_unused(req);
    boost::ignore_unused(resp);

    HTTPS_Client::req_resp_to_log(req, resp);
/*
    pt::ptree tree;
    std::istringstream bstr(resp.body());
    pt::read_json(bstr, tree);
    bool ok;

    for(auto&& rv : tree)
    {
        const std::string& key = rv.first; // key

        if("ok" == key)
            ok = ("true" == rv.second.get_value<std::string>("ok"));
        else if("result" == key)
            parse_result(rv.second);
    }
*/
}

void Telegram::parse_result(const pt::ptree& rpt)
{
    for(auto it = rpt.begin(); it != rpt.end(); ++it)
    {
        for(auto&& v : it->second)
        {
            const std::string& key = v.first;

            if("update_id" == key)
            {
                m_update_list.push_back(Update());
                m_update_list.back().update_id = v.second.get_value<int>();
                parse_update(it->second, m_update_list.back());
                m_last_update_id = m_update_list.back().update_id;
                m_event_handler(Events::EVENT_TLG_UPDATE_ID, boost::variant<int>(m_last_update_id));
            }
        }
    }
}

void Telegram::parse_update(const pt::ptree& upt, Update& update)
{
    for(auto&& sv : upt)
    {
        const std::string& key = sv.first; // key

        if(sv.second.empty())
            continue;

        if("message" == key || "edited_message" == key)
        {            
            parse_message(sv.second, update.message);
        }
     }
}

void Telegram::parse_message(const pt::ptree& mpt, Message& message)
{
    for(auto&& v : mpt)
    {
        const std::string& key = v.first;

        if("message_id" == key)
            message.message_id = v.second.get_value<int>();
        else if("date" == key)
            message.date = v.second.get_value<int>();
        else if("edit_date" == key)
            message.edit_date = v.second.get_value<int>();
        else if("text" == key)
            message.text = v.second.get_value<std::string>();
        else if(v.second.empty())
            continue;

        if("chat" == key)
            parse_chat(v.second, message.chat);
        else if("from" == key)
            parse_user(v.second, message.from);
        else if("entities" == key)
            parse_entities(v.second, message.entities);
    }
}

void Telegram::parse_chat(const pt::ptree& cpt, Chat& chat)
{
    for(auto&& v : cpt)
    {
        const std::string& key = v.first;

        if("id" == key)
        {
            chat.id = v.second.get_value<int>();
            m_chat_id = chat.id;
        }
        else if("first_name" == key)
            chat.first_name = v.second.get_value<std::string>();
        else if("last_name" == key)
            chat.last_name = v.second.get_value<std::string>();
        else if("type" == key)
            chat.type = get_chat_type(v.second.get_value<std::string>());
    }
}

void Telegram::parse_user(const pt::ptree& upt, User& user)
{
    for(auto&& v : upt)
    {
        const std::string& key = v.first;

        if("id" == key)
            user.id = v.second.get_value<int>();
        else if("first_name" == key)
            user.first_name = v.second.get_value<std::string>();
        else if("last_name" == key)
            user.last_name = v.second.get_value<std::string>();
        else if("is_bot" == key)
            user.is_bot = "true" == v.second.get_value<std::string>();
    }
}

void Telegram::parse_entities(const pt::ptree& ept, std::list<MessageEntity>& entities)
{
    for(auto it = ept.begin(); it != ept.end(); ++it)
    {
        entities.push_back(MessageEntity());

        for(auto&& v : it->second)
        {
            const std::string& key = v.first;

            if("offset" == key)
                entities.back().offset = v.second.get_value<int>();
            else if("length" == key)
                entities.back().length = v.second.get_value<int>();
            else if("type" == key)
                entities.back().type = get_entity_type(v.second.get_value<std::string>());
        }
    }
}

void Telegram::on_connect_update_client(bool connect)
{
    if(connect)
        mlogger::info("%s: %s", m_name.c_str(), "HTTPS UPD client, соединение - Ok");
    else
        mlogger::error("%s: %s", m_name.c_str(), "HTTPS UPD client, соединение - Ошибка");

    if(connect)
    {
        m_init_timer.async_wait(boost::bind(&Telegram::request_updates, this));
    }
}

void Telegram::on_connect_request_client(bool connect)
{
    if(connect)
        mlogger::info("%s: %s", m_name.c_str(), "HTTPS REQ client, соединение - Ok");
    else
        mlogger::error("%s: %s", m_name.c_str(), "HTTPS UPD client, соединение - Ошибка");
}

void Telegram::request_updates()
{
    pt::ptree pt;
    pt.put("timeout", m_poll_timeout);
    pt.put("offset", m_last_update_id + 1);

    client_poll.reset_requests();

    client_poll.request(http::verb::get,
                        m_target + "/getUpdates",
                        HTTPS_Client::ContentType::json,
                        pt);
}

void Telegram::update_handler(http::request<http::string_body>& req, http::response<http::string_body>& resp)
{
    boost::ignore_unused(req);

    std::cout << "===================================================================================" << std::endl;
    std::cout << resp << std::endl;
    std::cout << "===================================================================================" << std::endl;


    pt::ptree tree;
    std::istringstream bstr(resp.body());

    try
    {
        pt::read_json(bstr, tree);
    }
    catch (std::exception const& e)
    {
        mlogger::error("%s: Telegram parse json error:%s\n%s", m_name.c_str(), e.what(), resp.body().c_str());
        return;
    }
    //bool ok;

    for(auto&& rv : tree)
    {
        const std::string& key = rv.first; // key

//        if("ok" == key)
//            ok = ("true" == rv.second.get_value<std::string>("ok"));
        if("result" == key)
            parse_result(rv.second);
    }

    for(Update& update :  m_update_list)
    {
        switch(m_input_mode)
        {
            case IM_Command:
                break;

            case IM_Order:
                m_event_handler(Events::EVENT_TLG_ORDER_DATA,
                                boost::variant<std::string>(update.message.text));
                continue;

            case IM_Script:
                m_event_handler(Events::EVENT_TLG_SCRIPT_DATA,
                                boost::variant<std::string>(update.message.text));
                set_input_mode(IM_Command);
                break;

            case IM_Statistic:
                m_event_handler(Events::EVENT_TLG_STAT_DATA,
                                boost::variant<std::string>(update.message.text));
                break;

            default:
                continue;
        }

        for(MessageEntity& entity : update.message.entities)
        {
            if(entity.type == MessageEntity::entity_type::bot_command)
            {
                command_handler(update.message);
            }
        }
    }

    m_update_list.clear();
    request_updates();
}

void Telegram::command_handler(Message& msg)
{
    std::vector<std::string> args;    
    boost::algorithm::split(args, msg.text,
                            boost::is_any_of(" - "),
                            boost::algorithm::token_compress_on);

    std::string mlog = boost::algorithm::join(args, " ");
    mlogger::info("%s: Принята команда '%s'", m_name.c_str(), mlog.c_str());

    auto handler = command_map.find(args.at(0));
    if(command_map.end() != handler)
    {
        handler->second(args);
        return;
    }

    if(0 == msg.text.find("/start"))
    {
        send_message(msg.chat.id, Messages::start_message(), "HTML");
    }
    else if(0 == msg.text.find("/settings"))
        send_message(msg.chat.id, Messages::help_message(), "HTML");
    else if(0 == msg.text.find("/test"))        
    {
        m_event_handler(Events::EVENT_TLG_TEST, boost::variant<char>('\0'));
    }
    else
        send_message(msg.chat.id, "???");
}

void Telegram::on_command_menu(const std::vector<std::string>& args)
{
    boost::ignore_unused(args);
    m_event_handler(Events::EVENT_TLG_MENU, boost::variant<char>('\0'));
}

void Telegram::on_command_account(const std::vector<std::string>& args)
{
    boost::ignore_unused(args);
    m_event_handler(Events::EVENT_TLG_GET_ACCOUNT, boost::variant<char>('\0'));
}

void Telegram::on_command_price(const std::vector<std::string>& args)
{
    boost::ignore_unused(args);
    m_event_handler(Events::EVENT_TLG_GET_PRICE, boost::variant<char>('\0'));

}

void Telegram::on_command_indicators(const std::vector<std::string>& args)
{
    boost::ignore_unused(args);
    m_event_handler(Events::EVENT_TLG_GET_INDICATORS, boost::variant<char>('\0'));
}

void Telegram::on_command_stat(const std::vector<std::string>& args)
{
    boost::ignore_unused(args);
    m_event_handler(Events::EVENT_TLG_STATISTIC, boost::variant<char>('\0'));
}

void Telegram::on_command_order(const std::vector<std::string>& args)
{

    if(m_interface != IntfTlg)
        return;

    if(args.size() < 2)
        return;

    bnc_command_t cmd;

    if("list" == args.at(1))
    {
        cmd.cmd = bnc_command::LIST;
        cmd.type = bnc_type::ALL;
    }
    else if("open" == args.at(1))
    {
        cmd.cmd = bnc_command::LIST;
        cmd.type = bnc_type::OPENED;
    }
    else if("del" == args.at(1))
    {
        cmd.cmd = bnc_command::DELETE;
        cmd.arg = args.at(1);
    }
    else if("new" == args.at(1))
    {
        cmd.cmd = bnc_command::NEW;
        cmd.arg = args.at(1);
    }
    else
    {
        m_event_handler(Events::EVENT_TLG_ORDER, boost::variant<char>('\0'));
        return;
    }


    m_event_handler(Events::EVENT_TLG_ORDER_OPS, boost::variant<bnc_command_t>(cmd));

}

void Telegram::on_command_script(const std::vector<std::string>& args)
{
    if(args.size() < 2)
        return;

    m_event_handler(Events::EVENT_TLG_SCRIPT, boost::variant<std::string>(args.at(1)));
}

void Telegram::on_command_cancel(const std::vector<std::string>& args)
{
    boost::ignore_unused(args);
    send_message(chat_id(), "Отменено");
}
