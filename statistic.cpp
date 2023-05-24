#include "statistic.h"

Statistic::Statistic(Telegram& telegram,
                     std::vector<std::string>& symbols_list,
                     callback_event event_handler) : m_telegram(telegram),
                                                     m_symbols_list(symbols_list),
                                                     m_event_handler(event_handler)
{

}

void Statistic::start()
{
    m_state = SS_Start;
    m_telegram.set_input_mode(Telegram::IM_Statistic);
    engine();
}

void Statistic::engine(const std::string& data)
{
    switch(m_state)
    {
        case SS_Start:
            {
                m_cancel = false;

                std::vector<std::string> keys;
                TlgKeyboard keyboard;

                if(m_symbols_list.size() > 1)
                {
                    for(const auto& sit: m_symbols_list)
                        keys.push_back(sit);

                    keys.push_back("$a Все");
                }
                else if(m_symbols_list.size() == 1)
                {
                    m_state = SS_Symbol;
                    break;
                }
                else
                {
                    m_cancel = true;
                    m_state = SS_Ready;
                    break;
                }

                keys.push_back("$c Отмена");
                keyboard.add_keys(keys);
                m_telegram.send_message(m_telegram.chat_id(),
                                        "Отчет по валюте:",
                                        "",
                                        Telegram::Keyboard,
                                        keyboard.keyboard());

                m_state = SS_Symbol;
            }
            return;

        case SS_Symbol:
            {
                std::string cmdata = first_arg(data);

                if("$c" == cmdata)
                {
                    m_cancel = true;
                    m_state = SS_Ready;
                    break;
                }
                else if("$a" == cmdata)
                {
                    m_data.symbol = "";
                }
                else
                {
                    m_data.symbol = cmdata;
                }

                std::vector<std::string> keys;
                TlgKeyboard keyboard;

                keys.push_back("1 час");
                keys.push_back("2 часа");
                keys.push_back("3 часа");
                keys.push_back("12 часов");
                keys.push_back("24 часа");
                keys.push_back("48 часов");
                keys.push_back("Отмена");
                keyboard.add_keys(keys);

                m_telegram.send_message(m_telegram.chat_id(),
                                        "Статистика за период:",
                                        "",
                                        Telegram::MessageMode::Keyboard,
                                        keyboard.keyboard());
                m_state = SS_Period;
            }
            return;

        case SS_Period:
            {
                try
                {
                    m_data.hours = boost::lexical_cast<ushort>(first_arg(data));
                }
                catch (const boost::bad_lexical_cast& e)
                {
                    m_cancel = true;
                }

                m_state = SS_Ready;
            }
            break;

        case SS_Ready:
            if(m_cancel)
                m_telegram.send_message(m_telegram.chat_id(), "Отменено");
            else
                m_event_handler(Events::EVENT_STAT_READY, boost::variant<stat_data_t>(m_data));

            m_state = SS_Idle;
            m_telegram.set_input_mode(Telegram::IM_Command);
            return;

        default:
            return;
    }

    engine();
}

std::string Statistic::first_arg(const std::string& str)
{
    std::vector<std::string> args;
    boost::algorithm::split(args, str, boost::is_any_of(" "));
    return args.empty() ? "" : args.at(0);
}

