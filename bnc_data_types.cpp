#include "bnc_data_types.h"

std::map<BncOrder::Type, const std::string> BncOrder::type_map = {{BncOrder::Type::UNKNOWN, std::string("")},
                                                            {BncOrder::Type::LIMIT, std::string("LIMIT")},
                                                            {BncOrder::Type::MARKET, "MARKET"},
                                                            {BncOrder::Type::STOP_LOSS, "STOP_LOSS"},
                                                            {BncOrder::Type::STOP_LOSS_LIMIT, "STOP_LOSS_LIMIT"},
                                                            {BncOrder::Type::TAKE_PROFIT, "TAKE_PROFIT"},
                                                            {BncOrder::Type::LIMIT_MAKER, "LIMIT_MAKER"}};

std::map<BncOrder::TimeInForce, const std::string> BncOrder::tif_map = {{BncOrder::TimeInForce::UNK, std::string("")},
                                                                  {BncOrder::TimeInForce::FOK, std::string("FOK")},    // (Fill-Or-Kill) – Либо будет куплено все указанное количество немедленно,
                                                                                                                    // либо не будет куплено вообще ничего, ордер отменится.
                                                                  {BncOrder::TimeInForce::GTC, std::string("GTC")},    // (Good Till Cancelled) – ордер будет висеть до тех пор, пока его не отменят.
                                                                  {BncOrder::TimeInForce::IOC, std::string("IOC")}};   // (Immediate Or Cancel) – Будет куплено то количество, которое можно купить немедленно.
                                                                                                                    // Все, что не удалось купить, будет отменено.
