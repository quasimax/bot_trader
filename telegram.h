#ifndef TELEGRAM_H
#define TELEGRAM_H

#include <string>
#include <list>
#include <vector>
#include <map>
#include <queue>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/algorithm/string.hpp>

#include "https_client.h"
#include "tkeyboard.h"
#include "events.h"
#include "general.h"

namespace pt = boost::property_tree;

#define MAX_TEXT_MSG_SIZE   4096

class Telegram
{
    typedef struct
    {
        int id;
        std::string first_name;
        std::string last_name;
        bool is_bot;

    } User;    

    typedef struct
    {
        int id;
        enum chat_type{unknown, privat, group, supergroup, channel} type;
        std::string title;
        std::string username;
        std::string first_name;
        std::string last_name;

    } Chat;

    typedef struct
    {
        enum entity_type{unknown, mention,  hashtag, bot_command, url, email, bold, italic, code, pre, text_link} type;
        int offset;
        int length;

    } MessageEntity;

    typedef struct
    {
        int message_id;
        User from;
        Chat chat;
        uint32_t date;
        uint32_t edit_date;
        std::string text;
        bool group_chat_created;
        User new_chat_member;
        User left_chat_member;
        std::list<MessageEntity> entities;

    } Message;

    typedef struct
    {
        int update_id;
        Message message;

    } Update;

public:
    explicit Telegram(const std::string& name,
                      asio::io_context& ioc,
                      ssl::context& ctx,
                      uinterface& intf,
                      std::vector<std::string>& symbols_list,
                      callback_event event_handler);

    void stop();

    inline void setConnectionData(const std::string& host,
                                  short port,
                                  const std::string& token)
    {
        client_req.setConnectionData(host, port);
        client_poll.setConnectionData(host, port);
        m_target = "/bot" + token;
    }

    inline void setProxyData(const std::string& mode,
                             const std::string& host,
                             const std::string& port)
    {
        client_poll.setProxyData(mode, host, port);
        client_req.setProxyData(mode, host, port);
        client_poll.setProxyData(mode, host, port);
    }

    inline void set_settings(int last_update_id){m_last_update_id = last_update_id;}
    inline void set_alarm_channel_id(const std::string& alarm_channel_id){m_alarm_channel_id = alarm_channel_id;}    
    void request(const std::string& command);

    enum InputMode{IM_Command, IM_Order, IM_Line, IM_Script, IM_Statistic};
    inline void set_input_mode(InputMode mode){m_input_mode = mode;}

    enum MessageMode{TextOnly, Keyboard, Replay};
    void send_message(int chat_id,
                      const std::string& text,
                      const std::string& parse_mode = "",
                      MessageMode mode = TextOnly,
                      const pt::ptree& keyboard = {});

    void send_alarm_message(const std::string& text);

    inline int chat_id(){return m_chat_id;}
    inline std::queue<std::string>& last_commands(){return m_cmd_queue;}

private:    
    void init();
    void command_handler(Message& msg);    

    // callbacks
    void answer_handler(http::request<http::string_body>& req, http::response<http::string_body>& resp);
    void update_handler(http::request<http::string_body>& req, http::response<http::string_body>& resp);
    void on_connect_update_client(bool connect);
    void on_connect_request_client(bool connect);

    void request_updates();

    void parse_result(const pt::ptree& ppt);
    void parse_update(const pt::ptree& ppt, Update& update);
    void parse_message(const pt::ptree& mpt, Message& message);
    void parse_chat(const pt::ptree& cpt, Chat& chat);
    void parse_user(const pt::ptree& upt, User& user);
    void parse_entities(const pt::ptree& ept, std::list<MessageEntity>& entitys);

    void on_command_menu(const std::vector<std::string>& args);
    void on_command_account(const std::vector<std::string>& args);
    void on_command_price(const std::vector<std::string>& args);
    void on_command_indicators(const std::vector<std::string>& args);
    void on_command_order(const std::vector<std::string>& args);
    void on_command_script(const std::vector<std::string>& args);
    void on_command_stat(const std::vector<std::string>& args);
    void on_command_cancel(const std::vector<std::string>& args);

private:
    std::string m_name;
    asio::io_context& m_ioc;
    ssl::context& m_ctx;    
    std::string m_target;

    int m_poll_timeout{30};
    int m_last_update_id{0};
    int m_chat_id{0};
    std::string m_alarm_channel_id;
    uinterface& m_interface;
    std::vector<std::string>& m_symbols_list;

    http_req_resp_list m_req_resp_list;
    std::list<Update> m_update_list;

    callback_event m_event_handler;
    std::map<std::string, boost::function<void (const std::vector<std::string>&)>> command_map;

    boost::asio::deadline_timer m_init_timer;    
    InputMode m_input_mode{IM_Command};
    std::queue<std::string> m_cmd_queue;    

    HTTPS_Client client_req{"Telegram REQ HTTPS client",
                            m_ioc,
                            m_ctx,
                            boost::bind(&Telegram::on_connect_request_client, this, ::_1),
                            boost::bind(&Telegram::answer_handler, this, ::_1, ::_2)};


    HTTPS_Client client_poll{"Telegram UPD HTTPS client",
                             m_ioc,
                             m_ctx,
                             boost::bind(&Telegram::on_connect_update_client, this, ::_1),
                             boost::bind(&Telegram::update_handler, this, ::_1, ::_2)};







//----------------------------------------------------------------------------------------------------------

    // Chat types
    std::map<std::string, Chat::chat_type> chat_type_map = {std::pair<std::string, Chat::chat_type>("private", Chat::chat_type::privat),
                                                            std::pair<std::string, Chat::chat_type>("group", Chat::chat_type::group),
                                                            std::pair<std::string, Chat::chat_type>("supergroup", Chat::chat_type::supergroup),
                                                            std::pair<std::string, Chat::chat_type>("channel", Chat::chat_type::channel)};

    inline Chat::chat_type get_chat_type(const std::string& ctype)
    {
        std::map<std::string, Chat::chat_type>::iterator it = chat_type_map.find(ctype);
        return chat_type_map.end() != it ? it->second : Chat::chat_type::unknown;
    }

    // Entyty types
    std::map<std::string, MessageEntity::entity_type> entity_type_map = {std::pair<std::string, MessageEntity::entity_type>("pre", MessageEntity::entity_type::pre),
                                                                         std::pair<std::string, MessageEntity::entity_type>("url", MessageEntity::entity_type::url),
                                                                         std::pair<std::string, MessageEntity::entity_type>("bold", MessageEntity::entity_type::bold),
                                                                         std::pair<std::string, MessageEntity::entity_type>("code", MessageEntity::entity_type::code),
                                                                         std::pair<std::string, MessageEntity::entity_type>("email", MessageEntity::entity_type::email),
                                                                         std::pair<std::string, MessageEntity::entity_type>("italic", MessageEntity::entity_type::italic),
                                                                         std::pair<std::string, MessageEntity::entity_type>("hashtag", MessageEntity::entity_type::hashtag),
                                                                         std::pair<std::string, MessageEntity::entity_type>("mention", MessageEntity::entity_type::mention),
                                                                         std::pair<std::string, MessageEntity::entity_type>("mention", MessageEntity::entity_type::mention),
                                                                         std::pair<std::string, MessageEntity::entity_type>("bot_command", MessageEntity::entity_type::bot_command),
                                                                         std::pair<std::string, MessageEntity::entity_type>("text_link", MessageEntity::entity_type::text_link)};

    inline MessageEntity::entity_type get_entity_type(const std::string& etype)
    {
        std::map<std::string, MessageEntity::entity_type>::iterator it = entity_type_map.find(etype);
        return entity_type_map.end() != it ? it->second : MessageEntity::entity_type::unknown;
    }
};

#endif // TELEGRAM_H
