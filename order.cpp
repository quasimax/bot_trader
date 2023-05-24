#include "order.h"

TOrder::TOrder(Telegram& telegram, std::vector<std::string>& symbols_list, callback_event event_handler) : m_telegram{telegram},
                                                                                                         m_symbols_list(symbols_list),
                                                                                                         m_event_handler{event_handler}
{
    m_view.insert(std::make_pair("symbol", std::make_pair("пара: ", "")));
    m_view.insert(std::make_pair("newClientOrderId", std::make_pair("имя: ", "")));
    m_view.insert(std::make_pair("side", std::make_pair("операция: ", "")));
    m_view.insert(std::make_pair("quantity", std::make_pair("количество: ", "")));
    m_view.insert(std::make_pair("price", std::make_pair("цена: ", "")));
    m_view.insert(std::make_pair("stopPrice", std::make_pair("стоп - цена: ", "")));
    m_view.insert(std::make_pair("type", std::make_pair("тип: ", "")));
    m_view.insert(std::make_pair("timeInForce", std::make_pair("вид: ", "")));
}

void TOrder::del()
{
    m_state = OS_Delete;
    m_telegram.set_input_mode(Telegram::IM_Order);
    engine(std::string());
}

void TOrder::list()
{
    m_state = OS_List;
    m_telegram.set_input_mode(Telegram::IM_Order);
    engine(std::string());
}

void TOrder::start()
{
    m_state = OS_Start;
    m_telegram.set_input_mode(Telegram::IM_Order);
    engine(std::string());
}

void TOrder::prev()
{
    int st = static_cast<int>(m_state) - 1;
    m_state = static_cast<State>(st);
}

void TOrder::print()
{
    std::string str;

    str =  "------------------------------\n";
    str += std::string("НОВЫЙ ОРДЕР") + '\n';
    str += "------------------------------\n";

    for(const auto& line : m_view)
        str += line.second.first + line.second.second + '\n';

    str += "------------------------------\n";

    m_telegram.send_message(m_telegram.chat_id(), str);
}

void TOrder::reset_view()
{
    for(auto& line : m_view)
        line.second.second.clear();
}

std::string TOrder::first_arg(const std::string& str)
{
    std::vector<std::string> args;
    boost::algorithm::split(args, str, boost::is_any_of(" "));
    return args.empty() ? "" : args.at(0);
}

bool TOrder::input_double(const std::string val, double& field, const std::string& name, const std::string& emsg)
{
    bool result{true};

    try
    {
        field = boost::lexical_cast<double>(val);
        m_view[name].second = val;
    }
    catch (const boost::bad_lexical_cast &e)
    {
        m_telegram.send_message(m_telegram.chat_id(),
                                emsg,
                                "",
                                Telegram::Replay);
        result = false;
    }

    return result;
}

void TOrder::engine(const std::string& data)
{
    switch(m_state)
    {
        case OS_Start:
            if(m_symbols_list.empty())
            {
                m_telegram.set_input_mode(Telegram::IM_Command);
                return;
            }

            reset_view();
            m_telegram.send_message(m_telegram.chat_id(),
                                    "Введите имя ордера",
                                    "",
                                    Telegram::Replay);
            set_state(OS_Name);
            return;

        case OS_Name:
            if(!m_repeat)
            {
                m_order.newClientOrderId = data;
                m_view["newClientOrderId"].second = data;
                print();
            }
            else
            {
                m_repeat = false;
            }

            if(m_symbols_list.size() > 1)
            {
                for(const auto& sit: m_symbols_list)
                    keys.push_back(sit);

                m_keyboard.add_keys(keys);
                m_telegram.send_message(m_telegram.chat_id(),
                                        "Выбор валюты:",
                                        "",
                                        Telegram::Keyboard,
                                        m_keyboard.keyboard());

                set_state(OS_Symbol);
            }
            else
            {
                m_order.symbol = *m_symbols_list.begin() + "USDT";
                m_view["symbol"].second = m_order.symbol;
                set_state(OS_InputType);
                break;
            }
            return;

        case OS_Symbol:
            m_order.symbol = first_arg(data) + "USDT";
            m_view["symbol"].second = m_order.symbol;
            set_state(OS_InputType);
            break;

        case OS_InputType:
            keys.clear();
            m_keyboard.clear();
            keys.push_back("LIMIT");
            keys.push_back("MARKET");
            m_keyboard.add_keys(keys);
            m_telegram.send_message(m_telegram.chat_id(),
                                    "Тип ордера?",
                                    "",
                                    Telegram::Keyboard,
                                    m_keyboard.keyboard());
            set_state(OS_Type);
            return;

        case OS_Type:
            if(first_arg(data) == "$h")
            {
                help();
                return;
            }
            m_order.type = BncOrder::string_to_type(first_arg(data));
            m_view["type"].second = first_arg(data);
            keys.clear();
            m_keyboard.clear();
            keys.push_back("BUY - покупка");
            keys.push_back("SELL - продажа");
            m_keyboard.add_keys(keys);
            m_telegram.send_message(m_telegram.chat_id(),
                                    "Тип операции",
                                    "",
                                    Telegram::Keyboard,
                                    m_keyboard.keyboard());
            set_state(OS_Side);
            return;

        case OS_Side:
            if(!m_repeat)
            {
                m_order.side = BncOrder::string_to_side(first_arg(data));
                m_view["side"].second = first_arg(data);
            }
            else
            {
                m_repeat = false;
            }
            print();
            m_telegram.send_message(m_telegram.chat_id(),
                                    "Введите количество",
                                    "",
                                    Telegram::Replay);
            set_state(OS_Quantity);
            return;

        case OS_Quantity:
            if(!input_double(data, m_order.quantity, "quantity", "Ошибка ввода количества"))
                return;

            print();

            if(m_order.type == BncOrder::MARKET)
            {
                set_state(OS_OrderReady);
                break;
            }

            if(m_order.type == BncOrder::LIMIT)
//            || m_order.type == BncOrder::STOP_LOSS_LIMIT
//            || m_order.type == BncOrder::TAKE_PROFIT_LIMIT
//            || m_order.type == BncOrder::LIMIT_MAKER)
            {
                set_state(OS_Price);
                m_telegram.send_message(m_telegram.chat_id(),
                                        "Введите цену",
                                        "",
                                        Telegram::Replay);
            }
            else
            {
                set_state(OS_StopPrice);
                m_telegram.send_message(m_telegram.chat_id(),
                                        "Введите стоп - цену",
                                        "",
                                        Telegram::Replay);
            }
            return;

        case OS_StopPrice:
            if(!m_repeat)
            {
                if(!input_double(data, m_order.stopPrice, "stopPrice", "Ошибка ввода стоп-цены"))
                    return;

                print();
            }
            else
            {
                m_repeat = false;
            }

            if(m_order.type == BncOrder::STOP_LOSS_LIMIT
            || m_order.type == BncOrder::TAKE_PROFIT_LIMIT)
            {
                set_state(OS_Price);
                m_telegram.send_message(m_telegram.chat_id(),
                                        "Введите цену",
                                        "",
                                        Telegram::Replay);
            }
            else
            {
                set_state(OS_TimeInForce);
                keys.clear();
                m_keyboard.clear();
                keys.push_back("GTC");
                keys.push_back("IOC");
                keys.push_back("FOK");
                keys.push_back("$h ОПИСАНИЕ");
                m_keyboard.add_keys(keys);
                m_telegram.send_message(m_telegram.chat_id(),
                                        "Время действия ордера?",
                                        "",
                                        Telegram::Keyboard,
                                        m_keyboard.keyboard());
            }
            return;

        case OS_Price:
            if(!m_repeat)
            {
                if(!input_double(data, m_order.price, "price", "Ошибка ввода цены"))
                    return;

                print();
            }
            else
            {
                m_repeat = false;
            }
/*
            if(m_correction_mode)
            {
                set_state(OS_WaitCorrect);
                break;
            }
*/

            if(m_order.type == BncOrder::LIMIT)
            //|| m_order.type == BncOrder::STOP_LOSS_LIMIT
            //|| m_order.type == BncOrder::TAKE_PROFIT_LIMIT)
            {
                set_state(OS_TimeInForce);
                keys.clear();
                m_keyboard.clear();
                keys.push_back("GTC");
                keys.push_back("IOC");
                keys.push_back("FOK");
                m_keyboard.add_keys(keys);
                m_telegram.send_message(m_telegram.chat_id(),
                                        "Время действия ордера?",
                                        "",
                                        Telegram::Keyboard,
                                        m_keyboard.keyboard());
            }
            else
            {

            }
            return;


        case OS_TimeInForce:
            if(first_arg(data) == "$h")
            {
                help();
                return;
            }
            m_order.timeInForce = BncOrder::string_to_tif(first_arg(data));
            m_view["timeInForce"].second = first_arg(data);
            print();
            set_state(OS_OrderReady);
            break;

        case OS_OrderReady:
            keys.clear();
            m_keyboard.clear();
            keys.push_back("$e Выставить");
            //keys.push_back("$b Назад");
            keys.push_back("$c Отмена");
            m_keyboard.add_keys(keys);
            m_telegram.send_message(m_telegram.chat_id(),
                                    "Ордер готов",
                                    "",
                                    Telegram::Keyboard,
                                    m_keyboard.keyboard());

            set_state(OS_OrderResult);
            return;

        case OS_OrderResult:
            if("$e" == first_arg(data))
            {
                m_telegram.send_message(m_telegram.chat_id(), std::string("Ордер ") + m_order.newClientOrderId + " отправлен");
                m_event_handler(Events::EVENT_ORD_MANUAL, boost::variant<OrderResult>(OST_Execute));
                m_telegram.set_input_mode(Telegram::IM_Command);
            }
            else if("$b" == first_arg(data))
            {

            }
            else if("$c" == first_arg(data))
            {
                m_telegram.send_message(m_telegram.chat_id(), std::string("Ордер ") + m_order.newClientOrderId + " отменен");
                m_event_handler(Events::EVENT_ORD_MANUAL, boost::variant<OrderResult>(OST_Cancel));
                m_telegram.set_input_mode(Telegram::IM_Command);
            }
            else
            {

            }
            m_telegram.set_input_mode(Telegram::IM_Command);
            return;

        case OS_Delete:
            if(m_symbols_list.empty())
            {
                m_telegram.set_input_mode(Telegram::IM_Command);
                return;
            }

            m_telegram.send_message(m_telegram.chat_id(),
                                    "Введите имя ордера для удаления",
                                    "",
                                    Telegram::Replay);
            set_state(OS_DelName);
            return;

        case OS_DelName:
            m_order.newClientOrderId = data;

            if(m_symbols_list.size() > 1)
            {
                keys.clear();
                m_keyboard.clear();

                for(const auto& sit: m_symbols_list)
                    keys.push_back(sit);

                m_keyboard.add_keys(keys);
                m_telegram.send_message(m_telegram.chat_id(),
                                        "Выбор валюты:",
                                        "",
                                        Telegram::Keyboard,
                                        m_keyboard.keyboard());

                set_state(OS_DelSymbol);
                return;
            }
            else
            {
                m_data = *m_symbols_list.begin();
                set_state(OS_DelSymbol);
                break;
            }

        case OS_DelSymbol:
            m_order.symbol = data + "USDT";
            set_state(OS_DelReady);
            break;

        case OS_DelReady:
            m_event_handler(Events::EVENT_ORD_DELETE, boost::variant<bnc_order_t>(m_order));
            m_telegram.set_input_mode(Telegram::IM_Command);
            return;

        case OS_List:
            if(m_symbols_list.size() > 1)
            {
                keys.clear();
                m_keyboard.clear();

                for(const auto& sit: m_symbols_list)
                    keys.push_back(sit);

                m_keyboard.add_keys(keys);
                m_telegram.send_message(m_telegram.chat_id(),
                                        "Выбор валюты:",
                                        "",
                                        Telegram::Keyboard,
                                        m_keyboard.keyboard());

                set_state(OS_ListSymbol);
                return;
            }
            else
            {
                m_data = *m_symbols_list.begin();
                set_state(OS_ListSymbol);
                break;
            }

        case OS_ListSymbol:
            set_state(OS_Idle);
            m_event_handler(Events::EVENT_ORD_REQ_LIST, boost::variant<std::string>(data/* + "USDT"*/));
            m_telegram.set_input_mode(Telegram::IM_Command);
            break;

        default:
            return;

    }

    engine(m_data);
}

void TOrder::help()
{
    std::string help_msg;

    switch(m_state)
    {
        case OS_Type:
            help_msg += "LIMIT - \n";
            help_msg += "MARKET - подажа или покупка по рыночной цене\n";
            help_msg += "STOP_LOSS - исполнятся по рынку, как только будет достигнута стоп - цена\n";
            help_msg += "STOP_LOSS_LIMIT\n";
            help_msg += "TAKE_PROFIT - При достижении стоп-цены будет выполнен MARKET ордер.\n";
            help_msg += "TAKE_PROFIT_LIMIT\n";
            help_msg += "LIMIT_MAKER - отклонятся, если ордер при выставлении может выполниться по рынку\n";
            break;

        case OS_TimeInForce:
            help_msg += "GTC - ордер будет висеть до тех пор, пока его не отменят.\n";
            help_msg += "IOC - Будет куплено то количество, которое можно купить немедленно. Все, что не удалось купить, будет отменено.\n";
            help_msg += "FOK - Либо будет куплено все указанное количество немедленно, либо не будет куплено вообще ничего, ордер отменится.\n";
            break;

        default:
            return;
    }

    m_telegram.send_message(m_telegram.chat_id(), help_msg);
    set_state(m_prev_state);
    m_repeat = true;
    engine(std::string());
}



