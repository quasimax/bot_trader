#ifndef BNC_DATA_TYPES_H
#define BNC_DATA_TYPES_H

#include <string>
#include <map>
#include <vector>

#include "tkeyboard.h"

enum bnc_command {NEW, LIST, INFO, DELETE};
enum bnc_type {ALL, OPENED};

typedef struct
{
    bnc_command cmd;
    bnc_type type;
    std::string arg;

} bnc_command_t;


class BncOrder
{
public:
    enum Type {UNKNOWN, LIMIT, MARKET, STOP_LOSS, STOP_LOSS_LIMIT, TAKE_PROFIT, TAKE_PROFIT_LIMIT, LIMIT_MAKER};
    enum Side {BUY = 1, SELL};
    enum TimeInForce {UNK, GTC, IOC, FOK};

    static BncOrder::Type string_to_type(const std::string& st)
    {
        auto it = type_map.begin();

        for(; it != type_map.end(); ++it)
        {
            if(st == it->second)
                break;
        }

        return type_map.end() != it ? it->first : UNKNOWN;
    }

    static const std::string& type_to_string(BncOrder::Type tt)
    {
        auto it = type_map.find(tt);
        return type_map.end() != it ? it->second : type_map.find(UNKNOWN)->second;
    }

    inline static BncOrder::Side string_to_side(const std::string& st)
    {
        return st == "BUY" ? BUY : SELL;
    }

    inline static std::string side_to_string(BncOrder::Side side)
    {
        return side == BUY ? "BUY" : "SELL";
    }

    static BncOrder::TimeInForce string_to_tif(const std::string& stif)
    {
        auto it = tif_map.begin();

        for(; it != tif_map.end(); ++it)
        {
            if(stif == it->second)
                break;
        }

        return tif_map.end() != it ? it->first : UNK;
    }

    static const std::string& tif_to_string(BncOrder::TimeInForce tt)
    {
        auto it = tif_map.find(tt);
        return tif_map.end() != it ? it->second : tif_map.find(UNK)->second;
    }


private:
    static std::map<BncOrder::Type, const std::string> type_map;
    static std::map<BncOrder::TimeInForce, const std::string> tif_map;
};

typedef struct
{
    std::string symbol;
    std::string newClientOrderId;
    BncOrder::Side side;
    BncOrder::Type type;
    BncOrder::TimeInForce timeInForce;
    double quantity;
    double quoteOrderQty;
    double price;    
    double stopPrice;
    double icebergQty;
    uint64_t recvWindow;

} bnc_order_t;

enum OrderResult {OST_Cancel, OST_Execute, OST_Correct};
enum OrderAction {OA_Execute, OA_View, OA_Correction};

typedef struct
{
    bnc_order_t order;
    OrderAction action;

} bnc_order_act_t;

typedef struct
{
    std::string msg;
    std::vector<std::string> keys;
    TlgKeyboard::Side orientation;

} tlg_keyboard_t;

typedef struct
{
    std::string symbol;
    ushort hours;

} stat_data_t;


#endif // BNC_DATA_TYPES_H
