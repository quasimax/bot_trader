#ifndef EVENTS_H
#define EVENTS_H

#include <list>

#include <boost/variant.hpp>
#include <boost/function.hpp>

#include "bnc_data_types.h"
#include "strategyserver.h"

typedef boost::variant<int,
                       double,
                       std::string,
                       bnc_order_t,
                       bnc_command_t,
                       bnc_order_act_t,
                       tlg_keyboard_t,
                       stat_data_t,
                       OrderResult> event_data_t;

enum Events {
    EVENT_BNC_WSS_DATA,
    EVENT_BNC_PRICE,
    EVENT_BNC_ORDER_LIST,
    EVENT_BNC_ORDER_REPORT,
    EVENT_BNC_ACCOUNT,
    EVENT_BNC_BALANCE,
    EVENT_BNC_EXCHANGE_INFO,

    EVENT_TLG_TEST,
    EVENT_TLG_UPDATE_ID,
    EVENT_TLG_TEXT_PRINT,
    EVENT_TLG_GET_PRICE,
    EVENT_TLG_GET_ACCOUNT,
    EVENT_TLG_ORDER,
    EVENT_TLG_ORDER_OPS,
    EVENT_TLG_ORDER_DATA,
    EVENT_TLG_GET_INDICATORS,
    EVENT_TLG_STATISTIC,
    EVENT_TLG_STAT_DATA,
    EVENT_TLG_MENU,
    EVENT_TLG_SYMBOL,
    EVENT_TLG_SCRIPT,
    EVENT_TLG_SCRIPT_OPS,
    EVENT_TLG_SCRIPT_DATA,

    EVENT_TDV_ALERT,
    EVENT_STAT_READY,

    EVENT_ORD_COMPLETE,
    EVENT_ORD_DELETE,
    EVENT_ORD_MANUAL,
    EVENT_ORD_REQ_LIST,

    EVENT_SCR_CREATE_ORDER,
    EVENT_SCR_GET_PRICE,
    EVENT_SCR_ADD_STREAM,
    EVENT_SCR_DEL_STREAM,
    EVENT_SCR_REQ_COMMAND,
    EVENT_SCR_REQ_VALUE,
    EVENT_SCR_TRADE_STATE,

    EVENT_WEB_REQ_TRADE_STATE,
    EVENT_WEB_SET_TRADE_DATA,
    EVENT_WEB_REQ_TRADE_POS,
    EVENT_WEB_TRADE_RUN,
    EVENT_WEB_GET_FILTERS
    };

typedef std::list<std::pair<Events, event_data_t>> event_list_t;
typedef boost::function<void (Events, const event_data_t)> callback_event;

#endif // EVENTS_H
