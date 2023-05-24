#ifndef ORDER_H
#define ORDER_H

#include "telegram.h"

class TOrder
{
public:
    TOrder(Telegram& telegram, std::vector<std::string>& symbols_list, callback_event event_handler);
    void start();    
    void del();
    void list();
    inline const bnc_order_t& get(){return m_order;}
    inline void set_data(const std::string& data){engine(data);}

private:
    enum State{OS_Idle,
               OS_Start,
               OS_Name,
               OS_Side,
               OS_Price,
               OS_StopPrice,
               OS_Quantity,
               OS_Symbol,
               OS_InputType,
               OS_Type,
               OS_TimeInForce,
               OS_OrderReady,
               OS_OrderResult,
               OS_Delete,
               OS_DelName,
               OS_DelSymbol,
               OS_DelReady,
               OS_List,
               OS_ListSymbol,
               OS_ListReady
              };

    void engine(const std::string& data);
    void print();
    void reset_view();
    void prev();
    void help();
    bool input_double(const std::string val, double& field, const std::string& name, const std::string& emsg);
    inline void set_state(State st){m_prev_state = m_state; m_state = st;}
    std::string first_arg(const std::string& str);

private:
    State m_state{OS_Idle};
    State m_prev_state{OS_Idle};
    std::string m_data;
    TlgKeyboard m_keyboard;
    std::vector<std::string> keys;
    std::map<std::string, std::pair<std::string, std::string>> m_view;
    bnc_order_t m_order;    
    Telegram& m_telegram;
    std::vector<std::string>& m_symbols_list;

    bool m_repeat{false};

    callback_event m_event_handler;
};


#endif // ORDER_H
