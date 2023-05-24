#ifndef STATISTIC_H
#define STATISTIC_H

#include "telegram.h"
#include "tkeyboard.h"
#include "bnc_data_types.h"
#include "events.h"


class Statistic
{
    enum State {SS_Idle, SS_Start, SS_Symbol, SS_Period, SS_Ready};

public:
    Statistic(Telegram& telegram, std::vector<std::string>& symbols_list, callback_event event_handler);
    void start();
    inline void set_data(const std::string& data){engine(data);}

private:
    void engine(const std::string& data = std::string());
    std::string first_arg(const std::string& str);

private:
    State m_state;
    Telegram& m_telegram;
    std::vector<std::string>& m_symbols_list;
    callback_event m_event_handler;
    stat_data_t m_data;
    bool m_cancel{false};
};

#endif // STATISTIC_H
