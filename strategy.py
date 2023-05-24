debug = False

if debug == False:
    from StrategyFramework import *
else:
    import threading
    from enum import Enum

#from statemachine import StateMachine

import json
import sys
import time


#def main():
#    while True:
#        print("while loop!")
#        time.sleep(1)

#if __name__ == '__main__':
#    main()

class StateMachine:
    def __init__(self):
        self.handlers = {}
        self.startState = None
        self.endStates = []
        self.state = "FSM_ST_START"
        self.old_state = "FSM_ST_START";

    def add_state(self, name, handler, end_state=0):
        name = name.upper()
        self.handlers[name] = handler
        if end_state:
            self.endStates.append(name)

    def set_start(self, name):
        self.startState = name.upper()

    def set_state(self, st):        
        if self.state in self.endStates:
            print("reached ", self.state)
        else:
            self.old_state = self.state
            self.state = st

        print("## set_state run - {}/{}".format(self.state, st))

    def get_state(self):
        return self.state

    def run(self, arg):
#        if self.state == self.old_state:
#            return;
        if self.state in self.endStates:
            print("reached ", self.state)
        else:
            print("## state run - {}".format(self.state))
            self.handlers[self.state](arg)

#    def run(self, cargo):
#        try:
#            handler = self.handlers[self.startState]
#        except:
#            raise InitializationError("must call .set_start() before .run()")
#        if not self.endStates:
#            raise  InitializationError("at least one state must be an end_state")

#        while True:
#            (newState, cargo) = handler(cargo)
#            if newState.upper() in self.endStates:
#                print("reached ", newState)
#                break
#            else:
#                handler = self.handlers[newState.upper()]

if debug == True:
    class Type(Enum):
        MARKET = 1
        LIMIT  = 2

    class Side(Enum):
        BUY   = 1
        SELL  = 2

    class TimeInForce(Enum):
        GTC = 1
        IOC = 2
        FOK = 3

    class UserCommand(Enum):
        YES    = 1
        NO     = 2
        EXEC   = 3
        CANCEL = 4

    class Order():
        def __init__(self):
            self.symbol = "BTC"
            self.newClientOrderId = ""
            self.quantity = 0.00
            self.price = 0.00
            self.stopPrice = 0.00
            self.timeInForce = TimeInForce.GTC
            self.side = Side.SELL
            self.type = Type.LIMIT


    class StrategyInstance():
        def __init__(self, server):
            self.server = server
            self.timer = threading.Timer(0, self.stop_timer)

        def exec_order(self, order):
            print("DEBUG func: {}".format(sys._getframe().f_code.co_name))

        def delete_order(self, order):
            print("DEBUG func: {}".format(sys._getframe().f_code.co_name))
            print("DELETE ORDER: {}".format(order.newClientOrderId))

        def delete_all_orders(self):
            print("DEBUG func: {}".format(sys._getframe().f_code.co_name))            

        def send_message(self, msg, alw = False):
            print("DEBUG func: {}".format(sys._getframe().f_code.co_name))
            print("TLG MSG: {}".format(msg))

        def request_value(self, msg):
            print("DEBUG func: {}".format(sys._getframe().f_code.co_name))
            print("TLG USER VAL: {}".format(msg))

        def request_command(self, msg, keys, orient = "Vertical"):
            print("DEBUG func: {}".format(sys._getframe().f_code.co_name))
            print("TLG USER CMD: {}, {}".format(msg, keys))

        def create_timer(self, name, func, interval, recurring = False):
            print("DEBUG func: {}".format(sys._getframe().f_code.co_name))
            print("CREATE TIMER: name '{}', interval {}, func {}, recur {}".format(name, interval, func, recurring))
            self.timer = threading.Timer(interval, func)
            self.timer.start()
            
        def stop_timer(self, name):
            print("DEBUG func: {}".format(sys._getframe().f_code.co_name))
            print("STOP TIMER: name '{}'".format(name))
            self.timer.cancel()

        def to_log(self, msg):
            print("DEBUG func: {}".format(sys._getframe().f_code.co_name))
            print("BOT LOG MSG: {}".format(msg))

        def add_stream(self, stream):
            print("DEBUG func: {}".format(sys._getframe().f_code.co_name))
            print("BOT LOG MSG: {}".format(stream))

        def add_stream(self, stream):
            print("DEBUG func: {}".format(sys._getframe().f_code.co_name))
            print("BOT LOG MSG: {}".format(stream))

        def to_journal(self, msg):
            print("DEBUG func: {}".format(sys._getframe().f_code.co_name))
            print("BOT JOURNAL MSG: {}".format(msg))

        def trade_state(self, jdata):
            print("DEBUG func: {}".format(sys._getframe().f_code.co_name))
            print("BOT TRADE STATE: {}".format(jdata))

        def get_price(self, symbol):
            print("DEBUG func: {}".format(sys._getframe().f_code.co_name))


class Balance():
    def __init__(self):
        self.free = 0.00
        self.locked = 0.00

class Account():
    def __init__(self):
        self.maker_commission = 0.01
        self.taker_commission = 0.01
        self.balances = {'USDT': Balance(), 'BTC': Balance(), 'BNB': Balance()}

class SymbolData():
    def __init__(self):
        self.run_step = 0
        self.orders = [Order(), Order()]
        self.fsm = StateMachine()
        self.is_wait_price = False
        self.counter = 0
        self.price = [0.00, 0.00]
        self.trigger = [0.00, 0.00]
        self.symbol = symbol
        self.curr_trade_state = {"e":"tradeState",
                                 "s": "",
                                 "x":"data",
                                 "a":"wait",
                                 "m":"Торги остановлены. Ожидание торговых данных",
                                 "O":{"S":"","X":""}}

        self.fsm.set_start("FSM_ST_START")
        self.fsm.add_state("FSM_ST_CHANGE", self.fsm_change)
        self.fsm.add_state("FSM_ST_READY", self.fsm_ready)
        self.fsm.add_state("FSM_ST_WAIT_ORDER_CMPL", self.fsm_wait_order_compl)
        self.fsm.add_state("FSM_ST_WAIT_ORDER_FILL", self.fsm_wait_order_fill)
        self.fsm.add_state("FSM_ST_WAIT_ORDER_DEL", self.fsm_wait_order_del)
        self.fsm.add_state("FSM_ST_WAIT_PRICE", self.fsm_wait_price)
        self.fsm.add_state("FSM_ST_IN_PRICE", self.fsm_in_price)
        self.fsm.add_state("FSM_ST_RESUME", self.fsm_resume)
        self.fsm.add_state("FSM_ST_STOP", None, end_state = 1)

        self.fsm.set_state("FSM_ST_READY")

class Strategy(StrategyInstance):
    def __init__(self, server):        
        StrategyInstance.__init__(self, server)
        
        self.fsm = StateMachine()

        self.fsm.add_state("FSM_ST_START", self.fsm_start)
        self.fsm.add_state("FSM_ST_CHANGE", self.fsm_change)
        self.fsm.add_state("FSM_ST_READY", self.fsm_ready)        
        self.fsm.add_state("FSM_ST_WAIT_USER_COMMAND", self.fsm_wait_user_command)
        self.fsm.add_state("FSM_ST_WAIT_INP_TRIGGER", self.fsm_wait_input_triggr)
        self.fsm.add_state("FSM_ST_WAIT_INP_PRICE", self.fsm_wait_input_price)
        self.fsm.add_state("FSM_ST_WAIT_INP_QUANTITY", self.fsm_wait_input_quantity)
        self.fsm.add_state("FSM_ST_READY_TO_TRADE", self.fsm_wait_ready_to_trade)
        self.fsm.add_state("FSM_ST_START_TRADE", self.fsm_start_trade)
        self.fsm.add_state("FSM_ST_WAIT_ORDER_CMPL", self.fsm_wait_order_compl)
        self.fsm.add_state("FSM_ST_WAIT_ORDER_FILL", self.fsm_wait_order_fill)
        self.fsm.add_state("FSM_ST_WAIT_ORDER_DEL", self.fsm_wait_order_del)

        self.fsm.add_state("FSM_ST_WAIT_PRICE", self.fsm_wait_price)
        self.fsm.add_state("FSM_ST_IN_PRICE", self.fsm_in_price)

        self.fsm.add_state("FSM_ST_RESUME", self.fsm_resume)
        self.fsm.add_state("FSM_ST_STOP", None, end_state = 1)
        self.fsm.set_start("FSM_ST_START")

        self.alert_data = ""
        self.alert_dict = {}
        
        self.account = Account()

        jsonData = """ {"X" : "EMPTY", "c" : "no_order"} """
        self.report = json.loads(jsonData)
        
        self.is_wait_price = False
        self.counter = 0
        self.reinput = False

        self.input_step = 0
        self.run_step = 0
        
        self.symbol = "BTC"
        self.orders = [Order(), Order()]
        self.price = [0.00, 0.00]
        self.trigger = [0.00, 0.00]

        self.temp_orders = [Order(), Order()]
        self.temp_trigger = [0.00, 0.00]
        
        self.running = False
        self.change_mode = False
        self.wait_order_fill = Order()

        self.curr_trade_state = {"e":"tradeState",
                                 "s": self.symbol,
                                 "x":"data",
                                 "a":"wait",
                                 "m":"Торги остановлены. Ожидание торговых данных",
                                 "O":{"S":"","X":""}}
        
        self.fsm.set_state("FSM_ST_READY")

        ##----------------------------------------------------------------------------

        self.symbol_data = {}

    def eval(self):
       self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
       ## self.to_alert_channel("Test alert message")
       self.curr_trade_state["x"] = "data"
       self.curr_trade_state["a"] = "ready"
       self.curr_trade_state["m"] = "Данные для торгов готовы"
       self.trade_state(json.dumps(self.curr_trade_state))

    def on_trade_run(self, data):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        json_data = """{"c":"---"}"""
        try:
            json_data = json.loads(data)
        except ValueError:
            # json_out_data["c"] = "001"
            # json_out_data["m"] = "Ошибка формата JSON торговых данных"
            print("Trade data JSON parse error")

        print(json_data)

        self.input_step = 0

        if "start" == json_data["a"]:
            if "BUY" == json_data["p"]:
                self.run_step = 0
            elif "SELL" == json_data["p"]:
                self.run_step = 1
            else:
                return

            self.send_message("Торги запущены, ожидание цены..");
            self.to_journal("Торги запущены:")

            self.fsm.set_state("FSM_ST_START_TRADE")
            self.fsm.run(None)

        else:
            self.fsm.set_state("FSM_ST_RESUME")
            self.fsm.run(None)

    def get_trade_steps(self, symbol):
        self.to_log("FSM state: {} func: {} symbol {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name, symbol))
        slist = []
        q_symb = self.account.balances[symbol].free## + self.account.balances[self.symbol].locked
        q_usdt = self.account.balances['USDT'].free## + self.account.balances['USDT'].locked

        if 0 != self.orders[0].price and self.orders[0].quantity <= q_usdt/self.orders[0].price:
            slist.insert(0, "BUY")

        if 0 != self.orders[1].quantity and self.orders[1].quantity <= q_symb:
            slist.insert(0, "SELL")

        return slist

    def on_symbols_list(self, list):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))

        for s in list:
            symbol_data[s] = SymbolData(s)



    def is_script_running(self):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))        
        return self.running

    def to_all_logs(self, msg, alw = False):
        self.send_message(msg, alw)
        self.to_journal(msg)
        self.trade_state(json.dumps(self.curr_trade_state))

    def on_trade_data(self, data):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        json_in_data = """{"c":"---"}"""
        try:
            json_in_data = json.loads(data)
        except ValueError:
            json_out_data["c"] = "001"
            json_out_data["m"] = "Ошибка формата JSON торговых данных"
            print("Trade data JSON parse error")

        print(json_in_data)
        json_out_data = {}
        json_out_data["e"] = "tradeData"
        json_out_data["a"] = json_in_data["a"]

        if(self.symbol != json_in_data["s"]):
            json_out_data["c"] = "002"
            json_out_data["m"] = "Валюта {} не поддерживается".format(json_in_data["s"])
            self.trade_state(json.dumps(json_out_data))
            return;

        try:
            if("set" == json_in_data["a"]):
                self.trigger[0] = float(json_in_data["B"]["t"])
                self.orders[0].price = float(json_in_data["B"]["p"])
                self.trigger[1] = float(json_in_data["S"]["t"])
                self.orders[1].price = float(json_in_data["S"]["p"])
                self.orders[0].quantity = float(json_in_data["q"])
                self.curr_trade_state["x"] = "data"
                self.curr_trade_state["a"] = "ready"
                self.curr_trade_state["m"] = "Данные для торгов готовы"
                self.trade_state(json.dumps(self.curr_trade_state))

            json_out_data["q"] = str(self.orders[0].quantity)
            json_out_data["s"] = self.symbol
            json_out_data["r"] = "1s"
            json_out_data["B"] = {}
            json_out_data["B"]["t"] = str(self.trigger[0])
            json_out_data["B"]["p"] = str(self.orders[0].price)
            json_out_data["S"] = {}
            json_out_data["S"]["t"] = str(self.trigger[1])
            json_out_data["S"]["p"] = str(self.orders[1].price)

            json_out_data["c"] = "000"
            json_out_data["m"] = "Ok"

        except KeyError:
            json_out_data["c"] = "003"
            json_out_data["m"] = "Валюта {} не поддерживается".format(json_in_data["s"])

        self.trade_state(json.dumps(json_out_data))


    def on_get_trade_state(self, symbol):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        self.trade_state(json.dumps(self.curr_trade_state))

    def on_price_timer(self):
        self.get_price(self.symbol)

    def reset_all(self):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        self.reset_order_report()
        self.is_wait_price = False
        self.input_step = 0
        self.run_step = 0
        self.reinput = False

    def reset_order_report(self):
        jsonData = """ {"X" : "EMPTY", "c" : "", "p" : 0, "q" : 0, "n" : 0} """
        self.report = json.loads(jsonData)


    def on_alert(self, name, body):
        #self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        tmp_dict = dict(x.split("=") for x in body.split("\n"))

        if tmp_dict["label"] == "MACD" :
            tmp_dict["Итог: "] = "Покупать" if float(tmp_dict["MACD"]) > 0.00 else "Продавать"

        if tmp_dict["label"] == "Momentum" :
            tmp_dict["Итог: "] = "Покупать" if float(tmp_dict["mom"]) > 0.00 else "Продавать"

        if tmp_dict["label"] == "HMA" :
            tmp_dict["Итог: "] = "Покупать" if float(tmp_dict["hullma"]) < float(tmp_dict["price"]) else "Продавать"

        self.alert_dict[tmp_dict["label"]] = tmp_dict

    def on_indicators(self):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        for key, value in self.alert_dict.items():
            msg = self.dict_to_string(value)
            self.send_message(msg)

#    def sself.to_logf(self, buf, fmt, *args):
#    buf.write(fmt % args)

    def dict_to_string(self, dict):
        str = ""
        for key, value in dict.items():
            if "label" == key:
                continue
            if "title" == key:
                key = "Индикатор:"

            str += key + " " + value + "\n"

        return str

    def percent(self, val, prc):
        return val/100 * prc

    def on_start(self):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        if "FSM_ST_READY" != self.fsm.get_state() and "FSM_ST_STOP" != self.fsm.get_state():
            self.send_message("FSM: Невозможно выполнить: скрипт работает")
        else:
            self.fsm.set_state("FSM_ST_START")            
            self.fsm.run(None)

    def on_change(self):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))

        self.is_wait_price = False
            
        self.fsm.set_state("FSM_ST_CHANGE")
        self.fsm.run(None)

    def on_stop(self):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        self.send_message("FSM: Скрипт остановлен пользователем", True)
        
        if "" != self.wait_order_fill.newClientOrderId:
            self.delete_order(self.wait_order_fill)
            self.fsm.set_state("FSM_ST_WAIT_ORDER_DEL")
        else:
            self.fsm.set_state("FSM_ST_RESUME")
            self.fsm.run(None)


    def on_set_symbol(self, symbol):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        self.symbol = symbol
        self.account.balances[symbol] = Balance()
        
    def on_account(self, acc):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        json_acc = json.loads(acc)
        print("### on_account\n:%s" % json_acc)
        try:
            self.account.taker_commission = float(json_acc["takerCommission"])/100
            self.account.maker_commission = float(json_acc["makerCommission"])/100
            balances = json_acc["balances"]

            self.to_log("Формат аккаунта REST_API")

            for elem in balances:
                s = elem["asset"]
                if s == self.symbol or s == "USDT":
                    self.account.balances[s].free = float(elem["free"])
                    self.account.balances[s].locked = float(elem["locked"])
                    self.to_log("Account: {} free {} / locked {}".format(s, self.account.balances[s].free, self.account.balances[s].locked))
        except KeyError:
            self.to_log("Непонятный формат аккаунта")


    def on_balance(self, bal):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        jbal = json.loads(bal)
        balances = jbal["B"]

        for symbol in balances:
            s = symbol['a']
            if  s == self.symbol or s == "USDT":
                self.account.balances[s].free = float(symbol['f'])
                self.account.balances[s].locked = float(symbol['l'])
                print("Balance: {} free {} / locked {}".format(s, self.account.balances[s].free, self.account.balances[s].locked))

    def on_wss_data(self, data):
        wss_data = json.loads(data)
        print("WSS DATA: %s" % data)

    def on_order_report(self, rep):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        self.report = json.loads(rep)
        self.to_log("ORDER EXEC REPORT: {}".format(rep))
        self.fsm.run(None)

    def on_price(self, data):
        #self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        pdata = json.loads(data)
        print("on_price: {} {}".format(pdata['symbol'], pdata['price']))
        symbol = pdata['symbol'].replace("USDT", '')
        if symbol == self.symbol:
            self.price.pop()            
            self.price.insert(0, float(pdata['price']))
        
        if True == self.is_wait_price:
            self.fsm.run(None)
            
    def on_user_value(self, val):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        self.fsm.run(val)

    def on_order_complete_timeout(self):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        self.send_message("FSM: Нет подтверждения ордера '{}' от Binance".format(self.orders[self.run_step].newClientOrderId))
        self.curr_trade_state["x"] = "order"
        self.curr_trade_state["a"] = "error"
        self.curr_trade_state["m"] = "Нет подтвердениея ордера от Binance"
        self.fsm.set_state("FSM_ST_RESUME")
        self.fsm.run(None)

    def on_order_fill_timeout(self):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        self.send_message("FSM: Не выполнен вовремя ордер '{}' от Binance".format(self.orders[self.run_step].newClientOrderId))
        self.fsm.set_state("FSM_ST_RESUME")
        self.fsm.run(None)        

    def is_trigger_in_price(self, symbol, val):
        if 0.00 == self.price[0] or 0.00 == self.price[1]:
            return False

        return (val >= self.price[0] and val <= self.price[1]) or (val <= self.price[0] and val >= self.price[1])
        

    def print_trade_pos(self):
        if False == self.running:
            str = "Введенные позиции:\n"
            str += "----------------------------\n"
            str += "USDT -> %s\n" % self.symbol
            str += "Kурс триггер: %f\n" % self.trigger[0]
            str += "Kурс операции: %f\n" % self.orders[0].price
            str += "Количество: %f\n" % self.orders[0].quantity
            str += "%s -> USDT\n" % self.symbol
            str += "Kурс триггер: %f\n" % self.trigger[1]
            str += "Kурс операции: %f\n" % self.orders[1].price
            str += "Количество: %f\n" % self.orders[1].quantity
            str += "----------------------------\n"
        else:
            str = "Новые позиции:\n"
            str += "----------------------------\n"
            str += "USDT -> %s\n" % self.symbol
            str += "Kурс триггер: %f\n" % self.temp_trigger[0]
            str += "Kурс операции: %f\n" % self.temp_orders[0].price
            str += "Количество: %f\n" % self.temp_orders[0].quantity
            str += "%s -> USDT\n" % self.symbol
            str += "Kурс триггер: %f\n" % self.temp_trigger[1]
            str += "Kурс операции: %f\n" % self.temp_orders[1].price
            str += "Количество: %f\n" % self.temp_orders[1].quantity
            str += "----------------------------\n"

        self.send_message(str)

    def reset_trade_pos(self):
        if True != self.running:
            self.trigger[0] = 0.00
            self.orders[0].price = 0.00
            self.orders[0].quantity = 0.00

            self.trigger[1] = 0.00
            self.orders[1].price = 0.00
            self.orders[1].quantity = 0.00
        else:
            self.temp_trigger[0] = 0.00
            self.temp_orders[0].price = 0.00
            self.temp_trigger[1] = 0.00
            self.temp_orders[1].price = 0.00

    def print_order_to_string(self, order):
        str = "------------------------------\n"
        str += "ОРДЕР: {}\n".format(order.newClientOrderId)
        str += "------------------------------\n"
        str += "пара: {}\n".format(order.symbol)
        str += "операция: {}\n".format(order.side)
        str += "количество: {}\n".format(order.quantity)
        str += "цена: {}\n".format(order.price)
        str += "тип: {}\n".format(order.type)
        str += "вид: {}\n".format(order.timeInForce)
        str += "------------------------------\n";
        return str

#================= FSM FUNCTIONS =====================#

    def fsm_ready(self, arg):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        #self.fsm.run(None)

    def fsm_start(self, arg):        
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))        

        self.reset_trade_pos()
        self.input_step = 0
        self.counter = 0

        msg = "Скрипт запущен"
        self.curr_trade_state["x"] = "script"
        self.curr_trade_state["a"] = "start"
        self.curr_trade_state["m"] = msg

        self.to_all_logs(msg, True)

        cmd_list = ["$1 Позиция USDT -> %s" % self.symbol, "$2 Позиция %s -> USDT" % self.symbol, "$q Количество %s" % self.symbol, "$c Отмена"]
        self.request_command("Ввод торговых данных:", cmd_list)
        ## self.request_command("Ввод торговых данных:", ["$1 Позиция USDT -> {}".format(self.symbol), "$c Отмена"])
        self.fsm.set_state("FSM_ST_WAIT_USER_COMMAND")

    def fsm_change(self, arg):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))

        self.temp_trigger[0] = self.trigger[0]
        self.temp_trigger[1] = self.trigger[1]
        self.temp_orders[0].price = self.orders[0].price
        self.temp_orders[1].price = self.orders[1].price
        self.temp_orders[0].quantity = self.orders[0].quantity
        self.temp_orders[1].quantity = self.orders[1].quantity
        
        self.print_trade_pos()
        self.fsm.set_state("FSM_ST_READY_TO_TRADE")
        self.fsm.run(None)        

    def fsm_wait_user_command(self, cmd):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        args = cmd.split(" ")
        if "$1" == args[0]:
            self.input_step = 0
            self.reinput = False
            self.request_value("Введите курс (триггер) USDT -> {}".format(self.symbol))
            self.fsm.set_state("FSM_ST_WAIT_INP_TRIGGER")
        elif "$2" == args[0]:
            self.input_step = 1
            self.reinput = False
            self.request_value("Введите курс (триггер) {} -> USDT".format(self.symbol))
            self.fsm.set_state("FSM_ST_WAIT_INP_TRIGGER")

        elif "$q" == args[0]:
            self.request_value("Введите количество {}".format(self.symbol))
            self.fsm.set_state("FSM_ST_WAIT_INP_QUANTITY")

        elif "$r" == args[0]:
            cmd_list = ["$1 Позиция USDT -> %s" % self.symbol, "$2 Позиция %s -> USDT" % self.symbol, "$q Количество %s" % self.symbol, "$e Запуск", "$c Отмена"]
            self.request_command("Ввод торговых данных:", cmd_list)
            self.fsm.set_state("FSM_ST_WAIT_USER_COMMAND")

        elif "$a" == args[0] or "$e" == args[0]:            
            if "$e" == args[0]:
                cmd_list = ["$r Назад", "$c Отмена"]
                
                q_btc = self.account.balances[self.symbol].free## + self.account.balances[self.symbol].locked
                q_usdt = self.account.balances['USDT'].free## + self.account.balances['USDT'].locked

                if self.orders[0].quantity <= q_usdt/self.orders[0].price:
                    cmd_list.insert(0, "$s1 USDT -> {}".format(self.symbol))

                if self.orders[1].quantity <= q_btc:
                    cmd_list.insert(0, "$s2 {} -> USDT".format(self.symbol))                 

                self.request_command("Шаг начала торгов", cmd_list)
                
            elif "$a" == args[0]:
                a_btc = self.account.balances[self.symbol].free + self.account.balances[self.symbol].locked
                a_usdt = self.account.balances['USDT'].free + self.account.balances['USDT'].locked

                cmd_list = ["$r Назад"]

                if self.orders[0].quantity <= a_usdt/self.orders[0].price:
                    cmd_list.insert(0, "$a1 USDT -> {}".format(self.symbol))

                if self.orders[1].quantity <= a_btc:
                    cmd_list.insert(0, "$a2 {} -> USDT".format(self.symbol))

                last = ""

                if 0 == self.run_step:
                    last = "USDT -> {}".format(self.symbol)
                else:
                    last = "{} -> USDT".format(self.symbol)

                self.request_command("Шаг продолжения торгов (сейчас {}):".format(last), cmd_list)
            
        elif "$c" == args[0]:
            msg = "Скрипт остановлен ползователем"
            self.curr_trade_state["x"] = "script"
            self.curr_trade_state["a"] = "stop"
            self.curr_trade_state["m"] = msg
            self.to_all_logs(msg)

            if "" != self.wait_order_fill.newClientOrderId:
                self.delete_order(self.wait_order_fill)
                self.fsm.set_state("FSM_ST_WAIT_ORDER_DEL")
            else:
                self.fsm.set_state("FSM_ST_RESUME")
                self.fsm.run(None)

        elif "$s1" == args[0] or "$s2" == args[0]:            
            self.input_step = 0
            self.run_step = int(args[0].strip("$s")) - 1
            
            self.send_message("Торги запущены, ожидание цены..");                
            self.to_journal("Торги запущены:")

            self.fsm.set_state("FSM_ST_START_TRADE")
            self.fsm.run(None)

        elif "$a1" == args[0] or "$a2" == args[0]:
            self.input_step = 0
            self.run_step = int(args[0].strip("$a")) - 1

            self.trigger[0] = self.temp_trigger[0]
            self.trigger[1] = self.temp_trigger[1]
            self.orders[0].price = self.temp_orders[0].price
            self.orders[1].price = self.temp_orders[1].price
            self.orders[0].quantity = self.temp_orders[0].quantity
            self.orders[1].quantity = self.temp_orders[1].quantity

            if "" != self.wait_order_fill.newClientOrderId:
                self.delete_order(self.wait_order_fill)
                self.fsm.set_state("FSM_ST_WAIT_ORDER_DEL")
                self.send_message("Торги продолжены c новыми данными, ожидание отмены ордера..")
                self.change_mode = True
            else:
                self.send_message("Торги продолжены c новыми данными, ожидание цены..")
                self.to_journal("Торги продолжены c новыми данными:")
                self.fsm.set_state("FSM_ST_START_TRADE")
                self.fsm.run(None)

        elif "$au" == args[0] or "$as" == args[0]:
            all_btc = 0.00

            if "$au" == args[0]:
                all_btc = self.account.balances['USDT'].free/self.orders[0].price

            if "$as" == args[0]:
               all_btc = self.account.balances[self.symbol].free

            # btc - self.percent(btc, self.account.taker_commission)

            if False == self.running:
                self.orders[0].quantity = all_btc
                self.orders[1].quantity = all_btc
            else:
                self.temp_orders[0].quantity = all_btc
                self.temp_orders[1].quantity = all_btc

            self.print_trade_pos()
            self.fsm.set_state("FSM_ST_READY_TO_TRADE")
            self.fsm.run(None)

    def fsm_wait_input_triggr(self, arg):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        try:
            if False == self.running:
                self.trigger[self.input_step] = float(arg)
            else:
                self.temp_trigger[self.input_step] = float(arg)

            if False == self.reinput:
                self.print_trade_pos()

            self.reinput = False
            self.fsm.set_state("FSM_ST_WAIT_INP_PRICE")
            self.request_value("Введите курс операции")
        except ValueError:
            self.request_value("Ошибка значения курса(триггера)!")
            self.fsm.set_state("FSM_ST_WAIT_USER_COMMAND")
            self.reinput = True
            self.fsm.run("$" + str(self.input_step + 1))

    def fsm_wait_input_price(self, arg):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        try:
            if False == self.running:
                self.orders[self.input_step].price = float(arg)
            else:
                self.temp_orders[self.input_step].price = float(arg)

            if False == self.reinput:
                self.print_trade_pos()

            self.reinput = False

            if False == self.running:
                ## self.fsm.set_state("FSM_ST_WAIT_INP_QUANTITY")
                ##self.fsm.set_state("FSM_ST_READY_TO_TRADE")
                ## msg = "Введите количество "
                ##if self.input_step == 0:
                ##    msg += self.symbol
                ##    self.request_value(msg)
                ##else:
                    self.fsm.set_state("FSM_ST_READY_TO_TRADE")
                    self.fsm.run(None)
            else:
                self.fsm.set_state("FSM_ST_READY_TO_TRADE")
                self.fsm.run(None)

        except ValueError:
            self.request_value("Ошибка значения курса!")
            self.fsm.set_state("FSM_ST_WAIT_INP_TRIGGER")
            self.reinput = True
            self.fsm.run(str(self.trigger[self.input_step]))

    def fsm_wait_input_quantity(self, arg):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        
        # free_usdt = self.account.balances['USDT'].free;
        # btc = 0.00

        # if False == self.running:
        #    btc = free_usdt/self.orders[self.input_step].price
        # else:
        #    free_usdt = free_usdt + self.account.balances['USDT'].locked
        #    btc = free_usdt/self.temp_orders[self.input_step].price

        try:
            quantity = float(arg)
            if False == self.running:
                self.orders[0].quantity = quantity
                self.orders[1].quantity = quantity
            else:
                self.temp_orders[0].quantity = quantity
                self.temp_orders[1].quantity = quantity

            cmd_list = ["$1 Позиция USDT -> %s" % self.symbol, "$2 Позиция %s -> USDT" % self.symbol, "$q Количество %s" % self.symbol, "$c Отмена"]
            self.request_command("Ввод торговых данных:", cmd_list)
            self.fsm.set_state("FSM_ST_READY_TO_TRADE")
            self.print_trade_pos()
            self.fsm.run(0)

        except ValueError:
            string = str(arg)
            if "ALL" == string.upper():
                cmd_list = []

                if 0.00 == self.orders[0].price:
                    self.send_message("Невозможно задать количество USDT - не задана цена USDT -> {}".format(self.symbol))
                elif self.account.balances['USDT']:
                    cmd_list.append("$au Все USDT")

                if 0.00 != self.account.balances[self.symbol]:
                    cmd_list.append("$as Все %s" % self.symbol)

                if 0 == len(cmd_list):
                    self.send_message("Нет доступных валют")
                    self.fsm.set_state("FSM_ST_READY_TO_TRADE")
                    self.fsm.run(None)
                else:
                    self.request_command("Выбор валюты:", cmd_list)
                    self.fsm.set_state("FSM_ST_WAIT_USER_COMMAND")

                return

                if self.input_step == 0:
                    if False == self.running:
                        self.orders[self.input_step].quantity = btc - self.percent(btc, self.account.taker_commission)
                        self.orders[self.input_step + 1].quantity = btc - self.percent(btc, self.account.taker_commission) * 2
                    else:
                        self.temp_orders[self.input_step].quantity = btc - self.percent(btc, self.account.taker_commission)
                        self.temp_orders[self.input_step + 1].quantity = btc - self.percent(btc, self.account.taker_commission) * 2

                self.fsm.set_state("FSM_ST_READY_TO_TRADE")
                
                if False == self.reinput:
                    self.print_trade_pos()
                    
                self.reinput = False
                self.fsm.run(None)
            else:
                self.request_value("Ошибка значения количества!")
                self.fsm.set_state("FSM_ST_WAIT_USER_COMMAND")
                self.reinput = True
                self.fsm.run("$q")

    def fsm_wait_ready_to_trade(self, arg):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        ## cmd_list = ["$1 Позиция USDT -> %s" % self.symbol, "$2 Позиция %s -> USDT" % self.symbol, "$c Остановить"]
        cmd_list = ["$1 Позиция USDT -> %s" % self.symbol, "$2 Позиция %s -> USDT" % self.symbol, "$q Количество %s" % self.symbol, "$c Отмена"]

        # all_btc = self.account.balances[self.symbol].free## + self.account.balances[self.symbol].locked
        # all_usdt = self.account.balances['USDT'].free## + self.account.balances['USDT'].locked 

        
        ### if self.orders[0].quantity <= all_usdt/self.orders[0].price:
        ###     cmd_list.insert(0, "$s1 USDT -> {}".format(self.symbol))

        ### if self.orders[0].quantity <= all_btc
        ###     cmd_list.insert(0, "$s2 {} -> USDT".format(self.symbol))

        ## btc = self.account.balances['USDT'].free/self.orders[self.run_step].price
        ## enough = (btc >= self.orders[self.input_step].quantity + self.percent(self.orders[self.input_step].quantity, self.account.taker_commission))

        ## if enough and self.orders[1].quantity != 0.0 and self.orders[1].price != 0.0:

        ## q_btc = self.account.balances[self.symbol].free
        ## q_usdt = self.account.balances['USDT'].free

        if False == self.running:
            if self.orders[0].quantity > 0 and self.orders[0].price > 0 and self.orders[1].price > 0:
                ## if self.orders[0].quantity <= q_usdt/self.orders[0].price or self.orders[1].quantity <= q_btc:
                cmd_list.append("$e Запуск")
        else:
            if self.temp_orders[0].quantity > 0 and self.temp_orders[0].price > 0 and self.temp_orders[1].price > 0:
                ## if (self.run_step == 0 and self.orders[0].quantity <= q_usdt/self.orders[0].price) or (self.run_step == 1 and self.orders[1].quantity <= q_btc):
                cmd_list.append("$a Применить изменения")

        self.request_command("Выбор действия", cmd_list)
        self.fsm.set_state("FSM_ST_WAIT_USER_COMMAND")

    def fsm_start_trade(self, arg):
        print("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        
        self.to_alert_channel("Торги {} запущены".format(self.symbol))
        self.to_journal("Количество {}: {}".format(self.symbol, self.orders[0].quantity))
        self.to_journal("USDT -> {}: триггер {}, цена {}".format(self.symbol, self.trigger[0], self.orders[0].price))
        self.to_journal("{} -> USDT: триггер {}, цена {}".format(self.symbol, self.trigger[1], self.orders[1].price))

        self.curr_trade_state["x"] = "trigger"
        self.curr_trade_state["a"] = "wait"
        self.curr_trade_state["m"] = "Ожидание триггера {}".format(self.trigger[self.run_step])
        self.trade_state(json.dumps(self.curr_trade_state))

        self.is_wait_price = True
        self.create_timer("price_timer", self.on_price_timer, 1, True)
        self.fsm.set_state("FSM_ST_WAIT_PRICE")
        self.running = True

    def fsm_wait_price(self, arg):
        print("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))

        if True == self.is_trigger_in_price(self.symbol, self.trigger[self.run_step]):
                self.is_wait_price = False                
                self.fsm.set_state("FSM_ST_IN_PRICE");
                self.fsm.run(None)

    def fsm_in_price(self, answ):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))

        msg = "Сработал триггер {}".format(self.trigger[self.run_step])
        self.curr_trade_state["x"] = "trigger"
        self.curr_trade_state["m"] = msg
        self.curr_trade_state["a"] = "ready"

        self.to_all_logs(msg)

        self.orders[self.run_step].symbol = self.symbol + "USDT"
        self.orders[self.run_step].stopPrice = 0

        if self.run_step == 1:
            self.orders[self.run_step].newClientOrderId = "order_{}_to_usdt_{}".format(self.symbol, str(self.counter))
            self.orders[self.run_step].side = Side.SELL
            self.curr_trade_state["O"]["S"] = "SELL"
            msg = "Выставлен ордер на продажу {} по {} USDT".format(self.symbol, self.orders[self.run_step].price)

        elif self.run_step == 0:
            self.orders[self.run_step].newClientOrderId = "order_usdt_to_{}_".format(self.symbol, str(self.counter))
            self.orders[self.run_step].side = Side.BUY
            self.curr_trade_state["O"]["S"] = "BUY"
            msg = "Выставлен ордер на покупку {} по {} USDT".format(self.symbol, self.orders[self.run_step].price)

        self.orders[self.run_step].type = Type.LIMIT
        self.orders[self.run_step].timeInForce = TimeInForce.GTC
        self.exec_order(self.orders[self.run_step])
        self.create_timer("compl_timer", self.on_order_complete_timeout, 5)
        ostr = self.print_order_to_string(self.orders[self.run_step])
        self.to_all_logs(ostr)
        self.fsm.set_state("FSM_ST_WAIT_ORDER_CMPL")

        self.curr_trade_state["m"] = msg
        self.curr_trade_state["x"] = "order"
        self.curr_trade_state["O"]["X"] = "NEW"

        self.to_all_logs(msg)

    def fsm_wait_order_compl(self, arg):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        try:
            code = int(self.report['code'])
            msg = "Ошибка выставления ордера к Binance:\nкод: {}\n сообщение: {}".format(code, str(self.report['msg']))

            self.curr_trade_state["a"] = "error"
            self.curr_trade_state["m"] = msg

            self.to_all_logs(msg)

            self.fsm.set_state("FSM_ST_RESUME")            
            self.reset_order_report()
            self.stop_timer()            
            self.fsm.run(None)

        except KeyError:            
            if self.orders[self.run_step].newClientOrderId == self.report['c'] and "NEW" == self.report['X']:
                self.reset_order_report()
                self.stop_timer("compl_timer")

                msg = "Ордер подтвержден Binance, ожидание выполнения"
                self.curr_trade_state["a"] = "wait"
                self.curr_trade_state["O"]["X"] = "FILLED"
                self.curr_trade_state["m"] = msg
                self.to_all_logs(msg)
                self.fsm.set_state("FSM_ST_WAIT_ORDER_FILL")
                self.wait_order_fill = self.orders[self.run_step]

    def fsm_wait_order_fill(self, arg):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        if self.orders[self.run_step].newClientOrderId == self.report['c'] and "FILLED" == self.report['X']:
            msg = "Ордер выполнен Binance"
            self.curr_trade_state["a"] = "ready"
            self.curr_trade_state["m"] = msg            

            self.to_all_logs(msg)
            self.to_log("Цикл {} шаг {}".format(self.counter, self.run_step))

            if self.run_step == 0:
                self.run_step = 1
            elif self.run_step == 1:
                self.run_step = 0
                self.counter = self.counter + 1                

            self.wait_order_fill.newClientOrderId = ""
            self.is_wait_price = True
            self.fsm.set_state("FSM_ST_WAIT_PRICE")

    def fsm_wait_order_del(self, arg):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        if "CANCELED" == self.report['X']:
            self.wait_order_fill.newClientOrderId = ""
            msg = "Ордер '{}' отменен".format(self.orders[self.run_step].newClientOrderId)
            self.to_all_logs(msg)
            
            self.curr_trade_state["x"] = "order"
            self.curr_trade_state["a"] = "stop"
            self.curr_trade_state["m"] = msg
            
            if True == self.change_mode:
                self.is_wait_price = True
                self.fsm.set_state("FSM_ST_WAIT_PRICE")
                self.change_mode = False                
            else:            
                self.fsm.set_state("FSM_ST_RESUME")
                self.fsm.run(None)

    def fsm_resume(self, arg):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))        
        self.stop_timer("price_timer")
        self.stop_timer("compl_timer")
        self.reset_all()
        self.running = False        
        msg = "Скрипт {} остановлен".format(self.symbol)
        self.fsm.set_state("FSM_ST_READY")
        self.curr_trade_state["x"] = "script"
        self.curr_trade_state["a"] = "stop"
        self.curr_trade_state["m"] = msg
        self.curr_trade_state["O"]["S"] = ""
        self.curr_trade_state["O"]["X"] = ""

        self.to_alert_channel(msg)
        self.to_all_logs(msg)

        self.fsm.run(None)       
