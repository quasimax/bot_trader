from mstrategy import Strategy
from mstrategy import Account
from mstrategy import Order
from mstrategy import UserCommand
import sys

strategy = Strategy(None)

args = []

# {"c": "order_btc_usdt_1", "X": "NEW"}
# {"c": "order_btc_usdt_1", "X": "FILLED"}
# {"c": "order_usdt_btc_1", "X": "NEW"}
# {"c": "order_usdt_btc_1", "X": "FILLED"}

#{"makerCommission": 10, "takerCommission": 10, "balances": [{"asset": "BTC", "free": "4723846.89208129", "locked": "0.00000000"}, {"asset": "LTC", "free": "4763368.68006011", "locked": "0.00000000"}]}

#{[{"a": "LTC", "f": "17366.18538083", "l": "0.00000000"}, {"a": "BTC", "f": "10537.85314051", "l": "2.19464093" }]}

while True:
    line = input("dbg >>")    
    args = line.split(" ")    
    if 0 == len(args):
        print("empty command!")
        continue
    
    cmd = args.pop(0)    
    
    if "exit" == cmd:
        print("script debugging stopped")
        break

    elif "start" == cmd:
        strategy.on_start()

    elif "val" == cmd:
        strategy.on_user_value(input("value >>"))

    elif "exec" == cmd:        
        strategy.on_order_report(input("json >>"))

    elif "stop" == cmd:
        strategy.on_stop()

    elif "symb" == cmd:
        strategy.on_symbols_list(["BNB", "BTC", "SXP"])

        strategy.on_account("""{"makerCommission": 10,
                                "takerCommission": 10,
                                "balances": [
                                                {
                                                    "asset": "BTC",
                                                    "free": "0.01200",
                                                    "locked": "0.00000000"
                                                },
                                                {
                                                    "asset": "LTC",
                                                    "free": "0.68006",
                                                    "locked": "0.00000000"
                                                },
                                                {
                                                    "asset": "USDT",
                                                    "free": "1001.8500",
                                                    "locked": "0.0000"
                                                }
                                            ]
                            }""")
        
        strategy.on_trade_data("""{ "c": "tradeData",
                                    "a": "set",
                                    "s": "BTC",
                                    "q": "0.002",
                                    "r": "1s",
                                    "B":
                                    {
                                        "t": "11200.00000000",
                                        "p": "11300.00000000"
                                    },
                                    "S":
                                    {
                                        "t": "10600.00000000",
                                        "p": "10630.00000000"
                                    }}""")
        
        
##        strategy.on_balance("""{"e": "outboundAccountInfo",
##                                "E": "1600182013507",
##                                "m": "10",
##                                "t": "10",
##                                "b": "0",
##                                "s": "0",
##                                "T": "true",
##                                "W": "true",
##                                "D": "true",
##                                "u": "1600182013506",
##                                "B": [
##                                         {
##                                             "a": "BNB",
##                                             "f": "0.99900000",
##                                             "l": "0.00000000"
##                                         },
##                                         {
##                                             "a": "USDT",
##                                             "f": "19.00000000",
##                                             "l": "14.00000000"
##                                         },
##                                         {
##                                             "a": "SXP",
##                                             "f": "0.01285521",
##                                             "l": "0.00000000"
##                                         }
##                                    ],
##                                "P": ["SPOT"]
##                                }""")
##
        plist = strategy.get_trade_steps("BTC")
        print("Trade positions: {}".format(plist))
        
        strategy.on_trade_run(""" {"c": "runTrade",
                                   "s": "BTC",
                                   "p": "BUY",
                                   "a": "start"
                                   } """)
        
        strategy.on_price(""" {"symbol": "BTCUSDT", "price":"11000"} """)
        strategy.on_price(""" {"symbol": "BNBUSDT", "price":"2300"} """)
        strategy.on_price(""" {"symbol": "BTCUSDT", "price":"11100"} """)
        strategy.on_price(""" {"symbol": "BTCUSDT", "price":"11250"} """)
##        strategy.on_order_report(""" {"code":-1013,
##                                      "msg":"Filter failure: MIN_NOTIONAL"}  """)
        
        strategy.on_order_report(""" {"c": "order_USDT_to_BTC_0",
                                      "X": "NEW",
                                      "s": "BTCUSDT"} """)

##        strategy.on_trade_run(""" {"c": "runTrade",
##                                   "s": "BTC",
##                                   "p": "SELL",
##                                   "a": "start"
##                                   } """)        

##        strategy.on_order_report(""" {"c": "xxx",
##                                      "X": "CANCELED",
##                                      "s": "BTCUSDT",
##                                     } """)
        
        strategy.on_order_report(""" {"c": "order_USDT_to_BTC_0",
                                      "X": "FILLED",
                                      "s": "BTCUSDT",
                                      "N": "USDT",
                                      "p": "10630",
                                      "n": "0.00012"  
                                      } """)
        
    elif "go" == cmd:
        strategy.on_balance("""{
    "e": "outboundAccountInfo",
    "E": "1600182013507",
    "m": "10",
    "t": "10",
    "b": "0",
    "s": "0",
    "T": "true",
    "W": "true",
    "D": "true",
    "u": "1600182013506",
    "B": [
        {
            "a": "BNB",
            "f": "0.99900000",
            "l": "0.00000000"
        },
        {
            "a": "USDT",
            "f": "19.00000000",
            "l": "14.00000000"
        },
        {
            "a": "SXP",
            "f": "0.01285521",
            "l": "0.00000000"
        }
    ],
    "P": [
        "SPOT"
    ]
}""")
##        strategy.on_set_symbol("BTC")
        strategy.on_account("""{"makerCommission": 10,
                                "takerCommission": 10,
                                "balances": [{"asset": "BTC", "free": "0.01200", "locked": "0.00000000"},
                                             {"asset": "LTC", "free": "0.68006", "locked": "0.00000000"},
                                             {"asset": "USDT", "free": "1001.8500", "l": "0.0000" }]}""")
##        strategy.on_account(input("json >>"))
##        strategy.on_start()
        strategy.on_user_value("$1")
        strategy.on_user_value("11200")
        strategy.on_user_value("11300")
        strategy.on_user_value("0.001")
        strategy.on_user_value("$2")
        strategy.on_user_value("10200")
        strategy.on_user_value("10100")
        strategy.on_user_value("$e")
        strategy.on_price(""" {"symbol": "BTC", "price":"11000"} """)
        strategy.on_price(""" {"symbol": "BTC", "price":"11100"} """)
        strategy.on_price(""" {"symbol": "BTC", "price":"11250"} """)
##        strategy.on_order_report("""{"c": "order_usdt_to_btc_0", "X": "NEW"}""")        
        strategy.on_change()
        strategy.on_user_value("$2")
        strategy.on_user_value("7200")
        strategy.on_user_value("7100")
        strategy.on_user_value("$a")
        strategy.on_order_report(""" {"c": "xxx", "X": "CANCELED"} """)
        strategy.on_price(""" {"symbol": "BTC", "price":"11000"} """)
        strategy.on_price(""" {"symbol": "BTC", "price":"11100"} """)
        strategy.on_price(""" {"symbol": "BTC", "price":"11250"} """)
        strategy.on_order_report(""" {"c": "order_usdt_to_btc_0", "X": "NEW"} """)
        strategy.on_order_report(""" {"c": "order_usdt_to_btc_0", "X": "FILLED"} """)

        
##        strategy.on_order_report(""" {"c": "order_btc_to_usdt_0", "X": "FILLED"} """)        
##        strategy.on_price(""" {"symbol": "BTC", "price":"11400"} """)
##        strategy.on_price(""" {"symbol": "BTC", "price":"10100"} """)        
##        strategy.on_order_report(""" {"c": "order_usdt_to_btc_0", "X": "NEW"} """)
##        strategy.on_order_report(""" {"c": "order_usdt_to_btc_0", "X": "FILLED", "q": 11000, "p": 0.0067, "n": 0.000067} """)

##        strategy.on_user_value("11200")
##        strategy.on_user_value("11300")
##        strategy.on_order_report(""" {"c": "order_btc_to_usdt_1", "X": "NEW"} """)
##        strategy.on_order_report(""" {"c": "order_btc_to_usdt_1", "X": "FILLED"} """)        
##        strategy.on_price(""" {"symbol": "BTC", "price":"11400"} """)
##        strategy.on_price(""" {"symbol": "BTC", "price":"10100"} """)        
##        strategy.on_order_report(""" {"c": "order_usdt_to_btc_1", "X": "NEW"} """)
##        strategy.on_order_report(""" {"c": "order_usdt_to_btc_1", "X": "FILLED", "q": 11000, "p": 0.0067, "n": 0.000067} """)
        
    elif "loop" == cmd:
        continue
    
    else:
        print("unknown command!")
