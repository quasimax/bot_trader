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
from decimal import *


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
        self.symbol = ""

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
            self.symbol = ""
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
            self.timer_func = None
            self.timer = None

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

        def create_timer(self, name, func):
            print("DEBUG func: {}".format(sys._getframe().f_code.co_name))
            print("CREATE TIMER: name '{}', func {}".format(name, func))
            self.timer_func = func

        def start_timer(self, name, interval):
            print("DEBUG func: {}".format(sys._getframe().f_code.co_name))
            print("START TIMER: name '{}', interval {}, func {}".format(name, interval, self.timer_func))
            self.timer = threading.Timer(interval, self.timer_func)
            self.timer.start()

            
        def stop_timer(self, name):
            print("DEBUG func: {}".format(sys._getframe().f_code.co_name))
            print("STOP TIMER: name '{}'".format(name))
            self.timer.cancel()

        def to_log(self, msg):
            print("DEBUG func: {}".format(sys._getframe().f_code.co_name))
            print("BOT LOG MSG: {}".format(msg))

        def to_alert_channel(self, msg):
            print("DEBUG func: {}".format(sys._getframe().f_code.co_name))
            print("ALERT CHANNEL MSG: {}".format(msg))            

        def add_stream(self, stream):
            print("DEBUG func: {}".format(sys._getframe().f_code.co_name))
            print("BOT LOG MSG: {}".format(stream))

        def add_stream(self, stream):
            print("DEBUG func: {}".format(sys._getframe().f_code.co_name))
            print("BOT LOG MSG: {}".format(stream))

        def to_journal(self, msg):
            print("DEBUG func: {}".format(sys._getframe().f_code.co_name))
            print("BOT JOURNAL MSG: {}".format(msg))

        def send_to_web(self, jdata):
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
        self.balances = {'USDT': Balance()}

class SymbolData():
    def __init__(self, symbol):
        self.step = 0
        self.orders = [Order(), Order()]

        self.orders[0].symbol = symbol + "USDT"
        self.orders[0].type = Type.LIMIT
        self.orders[0].timeInForce = TimeInForce.GTC
        self.orders[0].side = Side.BUY

        self.orders[1].symbol = symbol + "USDT"
        self.orders[1].type = Type.LIMIT
        self.orders[1].timeInForce = TimeInForce.GTC
        self.orders[1].side = Side.SELL        
        
        self.wait_order_fill = Order()
        
        self.fsm = StateMachine()
        self.fsm.set_start("FSM_ST_START")
        self.fsm.add_state("FSM_ST_READY", self.fsm_ready)
##        self.fsm.add_state("FSM_ST_CHANGE", self.fsm_change)
        self.fsm.add_state("FSM_ST_WAIT_PRICE", self.fsm_wait_price)
        self.fsm.add_state("FSM_ST_IN_PRICE", self.fsm_in_price)        
        self.fsm.add_state("FSM_ST_WAIT_ORDER_CMPL", self.fsm_wait_order_compl)
        self.fsm.add_state("FSM_ST_WAIT_ORDER_FILL", self.fsm_wait_order_fill)
        self.fsm.add_state("FSM_ST_WAIT_ORDER_DEL", self.fsm_wait_order_del)
        self.fsm.add_state("FSM_ST_RESUME", self.fsm_resume)
        self.fsm.add_state("FSM_ST_STOP", None, end_state = 1)        
        
        self.wait_price = False
        self.running = False
        self.change_mode = False
        self.counter = 0
        self.symbol = symbol
        self.price = [0.00, 0.00]
        self.trigger = [0.00, 0.00]        
        self.trade_state = {"e":"tradeState",
                            "s": symbol + "USDT",
                            "x":"data",
                            "a":"wait",
                            "m":"Торги остановлены. Ожидание торговых данных"}

        self.temp_price = [0.00, 0.00]        
        self.temp_trigger = [0.00, 0.00]
        self.temp_quantity = 0.00

        self.balance = Balance()
        self.balance_usdt = Balance()

        self.t_commission = 0.00
        self.m_commission = 0.00

        self.h_send_to_web = None
        self.h_to_log = None
        self.h_to_journal = None
        self.h_exec_order = None
        self.h_delete_order = None
        self.h_create_timer = None
        self.h_start_timer = None
        self.h_stop_timer = None
        self.h_to_alert_channel = None

        self.quotePrecision = 8
        self.quoteAssetPrecision = 8
        self.tickSize = 0.01
        self.stepSize = 0.000001
        self.minNotional = 10.00
        self.multiplierUp = 5
        self.multiplierDown = 0.2        

##        self.h_create_timer("{}_compl_timer".format.self.symbol)

    ## Event functions --------------------------------------------------------------------------------------

    def on_test(self):
        print("{} - FSM state: {} func: {}".format(self.symbol, self.fsm.get_state(), sys._getframe().f_code.co_name))
        print("### TEST")
        q = 0.000999
        n = 0.1571427
        p = 15730

        cmsn_symb = n / p
        q = q - cmsn_symb
        q = self.round_at_step(q, self.stepSize)
        print("new q = {}".format(q))


    def on_exchange_info(self, json_info):
        print("{} - FSM state: {} func: {}".format(self.symbol, self.fsm.get_state(), sys._getframe().f_code.co_name))        

        self.quotePrecision = int(json_info['quotePrecision'])
        self.quoteAssetPrecision = int(json_info['quoteAssetPrecision'])

        filters = json_info['filters']

        self.h_to_log("Filters {}:\n------------------------------------------".format(self.symbol))

        for filter in filters:
            if "PRICE_FILTER" == filter['filterType']:
                self.tickSize = float(filter['tickSize'])
                self.h_to_log("Filter: {} = {}".format(filter['filterType'], filter['tickSize']))
            elif "LOT_SIZE" == filter['filterType']:
                self.stepSize = float(filter['stepSize'])
                self.h_to_log("Filter: {} = {}".format(filter['filterType'], filter['stepSize']))
            elif "PERCENT_PRICE" == filter['filterType']:
                self.multiplierUp = float(filter['multiplierUp'])
                self.multiplierDown = float(filter['multiplierDown'])
                self.h_to_log("Filter: {} = Up {}, Down {}".format(filter['filterType'], filter['multiplierUp'], filter['multiplierDown']))
            elif "MIN_NOTIONAL" == filter['filterType']:
                self.minNotional = float(filter['minNotional'])
                self.h_to_log("Filter: {} = {}\n".format(filter['filterType'], filter['minNotional']))


    def on_trade_data(self, json_in_data):
        json_out_data = json_in_data        
        json_out_data["e"] = "tradeData"        
        
        try:
            if("set" == json_in_data["a"]):
                self.temp_trigger[0] = float(json_in_data["B"]["t"])
                self.temp_price[0] = float(json_in_data["B"]["p"])
                self.temp_trigger[1] = float(json_in_data["S"]["t"])
                self.temp_price[1] = float(json_in_data["S"]["p"])
                self.temp_quantity = float(json_in_data["q"])
                ## self.h_send_to_web(json.dumps(self.trade_state))

                if False == self.running:
                    self.fsm.set_state("FSM_ST_READY")
                    self.fsm.run(None)
                else:
                    self.trade_state["x"] = "script"
                    self.trade_state["a"] = "wait"
                    self.trade_state["m"] = "Данные изменены. Торги работают"

            self.h_send_to_web(json.dumps(self.trade_state))

            json_out_data["q"] = str(self.temp_quantity)
            json_out_data["r"] = "1s"
            json_out_data["B"] = {}
            json_out_data["B"]["t"] = str(self.temp_trigger[0])
            json_out_data["B"]["p"] = str(self.temp_price[0])
            json_out_data["S"] = {}
            json_out_data["S"]["t"] = str(self.temp_trigger[1])
            json_out_data["S"]["p"] = str(self.temp_price[1])

            json_out_data["c"] = "000"
            json_out_data["m"] = "Ok"

        except KeyError:
            json_out_data["c"] = "002"
            json_out_data["m"] = "JSON Ключ не найден"

        self.h_send_to_web(json.dumps(json_out_data))

    def on_trade_run(self, rjdata):
        go = (rjdata['a'] == "start")
        side = rjdata['p']
        
        if False == self.running:
            if True == go:
                # запуск сначала
                self.trigger[0] = self.temp_trigger[0]
                self.trigger[1] = self.temp_trigger[1]
                self.orders[0].price = self.temp_price[0]
                self.orders[1].price = self.temp_price[1]
                self.orders[0].quantity = self.temp_quantity
                self.orders[1].quantity = self.temp_quantity

                if "BUY" == side:
                    self.step = 0
                else:
                    self.step = 1

                self.running = True
                self.wait_price = True

                self.trade_state["x"] = "script"
                self.trade_state["a"] = "start"
                self.trade_state["m"] = "Торги запущены. Ожидание триггера {}-{}".format(self.symbol, side)
                self.h_send_to_web(json.dumps(self.trade_state))

                self.h_to_alert_channel("Торги {} запущены".format(self.symbol))
                self.h_to_journal(self.symbol, "Торги запущены")
                self.h_to_journal(self.symbol, self.print_trade_pos("Торговые данные"))
                
                self.fsm.set_state("FSM_ST_WAIT_PRICE")
                self.fsm.run(None)
            else:
                # стоп/стоп
                self.trade_state["x"] = "script"
                self.trade_state["a"] = "stop"
                self.trade_state["m"] = "Торги не запущены."
                self.h_send_to_web(json.dumps(self.trade_state))
                self.h_to_journal(self.symbol, "Запрос на остановку торгов")
        else:                
            if True == go:
                # изменение данных на ходу
                self.trigger[0] = self.temp_trigger[0]
                self.trigger[1] = self.temp_trigger[1]
                self.orders[0].price = self.temp_price[0]
                self.orders[1].price = self.temp_price[1]
                self.orders[0].quantity = self.temp_quantity
                self.orders[1].quantity = self.temp_quantity

                if "BUY" == side:
                    self.step = 0
                else:
                    self.step = 1

                msg = "Торги {} запущены c новыми данными".format(self.symbol)
                self.h_to_alert_channel(msg)
                self.h_to_journal(self.symbol, self.print_trade_pos("Торговые данные"))
                
                if True == self.wait_order_act():
                    self.change_mode = True
                    self.h_delete_order(self.wait_order_fill)
                    self.fsm.set_state("FSM_ST_WAIT_ORDER_DEL")

                    self.trade_state["x"] = "order"
                    self.trade_state["a"] = "wait"
                    msg = "Ожидание удаления ордера {}".format(self.current_order().newClientOrderId)
                    self.trade_state["m"] = msg
                else:
                    self.trade_state["x"] = "script"
                    self.trade_state["a"] = "start"
                    msg = "Торги {} запущены c новыми данными".format(self.symbol)
                    self.trade_state["m"] = msg

                self.h_to_journal(self.symbol, msg)
                self.h_send_to_web(json.dumps(self.trade_state))

            else:
                # остановка
                self.h_to_alert_channel("Торги {} остановлены пользователем".format(self.symbol))
                if True == self.wait_order_act():
                    self.h_delete_order(self.wait_order_fill)
                    self.fsm.set_state("FSM_ST_WAIT_ORDER_DEL")

                    self.trade_state["x"] = "order"
                    self.trade_state["a"] = "wait"
                    self.trade_state["m"] = "Ожидание удаления ордера {}".format(self.current_order().newClientOrderId)
                    self.h_send_to_web(json.dumps(self.trade_state))
                else:                    
                    self.fsm.set_state("FSM_ST_RESUME")
                    self.fsm.run(None)
                

    def on_price(self, price):
        print("{} - FSM state: {} func: {}".format(self.symbol, self.fsm.get_state(), sys._getframe().f_code.co_name))
        self.price.pop()            
        self.price.insert(0, float(price))
        
        if True == self.wait_price:
            self.fsm.run(None)

    def on_order_report(self, jreport):
        self.h_to_log("{} - FSM state: {} func: {}".format(self.symbol, self.fsm.get_state(), sys._getframe().f_code.co_name))                
        self.report = jreport
        if("FSM_ST_RESUME" != self.fsm.get_state()):
            self.fsm.run(None)

    def on_order_complete_timeout(self):
        self.h_to_log("{} - FSM state: {} func: {}".format(self.symbol, self.fsm.get_state(), sys._getframe().f_code.co_name))
        self.h_to_journal(self.symbol, "FSM: Нет подтверждения ордера {}".format(self.current_order().newClientOrderId))
        self.wait_order_fill = Order()
        
        self.trade_state["x"] = "order"
        self.trade_state["a"] = "error"
        self.trade_state["m"] = "Нет подтвердениея ордера от Binance"
        self.h_send_to_web(json.dumps(self.trade_state))
        
        self.h_to_alert_channel("Торги {} остановлены по таймауту".format(self.symbol))
        self.fsm.set_state("FSM_ST_RESUME")
        self.fsm.run(None)

    def get_trade_steps(self):
        self.h_to_log("{} - FSM state: {} func: {}".format(self.symbol, self.fsm.get_state(), sys._getframe().f_code.co_name))
        plist = []

        q_symb = self.balance.free + self.balance.locked
        q_usdt = self.balance_usdt.free + self.balance.locked

        if self.temp_price[0] > 0 and self.temp_quantity <= q_usdt/self.temp_price[0]:
            plist.append("BUY")
        
        if (self.temp_quantity) <= q_symb:
            plist.append("SELL")

        return plist            

    ## Common functions ---------------------------------------------------------------------------------------

    def percent(self, val, prc):
        return val/100 * prc

    def round_at_step(self, val, step):
        fstr = '%f' % step
        step_prec = len(fstr.split('.')[1])
        round_val = round(val, step_prec)
        if round_val >= val:
            round_val = round_val - step
        round_val = round(val, step_prec)
        return round_val

    def reset(self):
        self.h_to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        self.wait_price = False        
        self.step = 0        

    def wait_order_act(self):
        return (self.wait_order_fill.newClientOrderId != "")

    def current_order(self):
        return self.orders[self.step]

    def reset_order_report(self):
        jsonData = """ {"X" : "EMPTY", "c" : "", "p" : 0, "q" : 0, "n" : 0} """
        self.report = json.loads(jsonData)

    def is_trigger_in_price(self, val):
        if 0.00 == self.price[0] or 0.00 == self.price[1]:
            return False

        return ((val >= self.price[0] and val <= self.price[1]) or
                (val <= self.price[0] and val >= self.price[1]))

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

    def send_orders_change(self, msg):
        self.h_to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        ord_change = {'e': 'getFilters', 's': self.symbol, 'm': msg}
        ord_change['B'] = {'p': self.orders[0].price, 'q': self.orders[0].quantity}
        ord_change['S'] = {'p': self.orders[1].price, 'q': self.orders[1].quantity}
        self.h_send_to_web(json.dumps(ord_change))

    def print_trade_pos(self, header, temp = False):
        str = header + ":\n"
        str += "----------------------------\n"

        if False == temp:
            str += "USDT -> %s\n" % self.symbol
            str += "Kурс триггер: %f\n" % self.trigger[0]
            str += "Kурс операции: %f\n" % self.orders[0].price
            str += "Количество: %f\n" % self.orders[0].quantity
            str += "%s -> USDT\n" % self.symbol
            str += "Kурс триггер: %f\n" % self.trigger[1]
            str += "Kурс операции: %f\n" % self.orders[1].price
            str += "Количество: %f\n" % self.orders[1].quantity
        else:
            str += "USDT -> %s\n" % self.symbol
            str += "Kурс триггер: %f\n" % self.temp_trigger[0]
            str += "Kурс операции: %f\n" % self.temp_price[0]
            str += "Количество: %f\n" % self.temp_quantity
            str += "%s -> USDT\n" % self.symbol
            str += "Kурс триггер: %f\n" % self.temp_trigger[1]
            str += "Kурс операции: %f\n" % self.temp_price[1]
            str += "Количество: %f\n" % self.temp_quantity

        str += "----------------------------\n"
        return str


    ## FSM Functions ------------------------------------------------------------------------------------------

    
    def fsm_start(self, arg):        
        self.h_to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))

    def fsm_ready(self, arg):
        self.h_to_log("{} - FSM state: {} func: {}".format(self.symbol, self.fsm.get_state(), sys._getframe().f_code.co_name))
        self.trade_state["x"] = "data"
        self.trade_state["a"] = "ready"
        self.trade_state["m"] = "Данные для торгов готовы"        

    def fsm_wait_price(self, arg):
        print("{} - FSM state: {} func: {}".format(self.symbol, self.fsm.get_state(), sys._getframe().f_code.co_name))
        
        if True == self.is_trigger_in_price(self.trigger[self.step]):
            self.wait_price = False
            self.fsm.set_state("FSM_ST_IN_PRICE");
            self.fsm.run(None)

    def fsm_in_price(self, answ):
        self.h_to_log("FSM state: {} func: {} symbol {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name, self.symbol))

        msg = "Сработал триггер {}-{}".format(self.symbol, self.trigger[self.step])
        self.trade_state["x"] = "trigger"
        self.trade_state["a"] = "ready"
        self.trade_state["m"] = msg        
        
        self.h_send_to_web(json.dumps(self.trade_state))
        self.h_to_journal(self.symbol, msg)

        if self.step == 1:
            self.current_order().newClientOrderId = "order_{}_to_USDT_{}".format(self.symbol, str(self.counter))            
            msg = "Выставлен ордер на продажу {} по {} USDT".format(self.symbol, self.current_order().price)
        elif self.step == 0:
            self.current_order().newClientOrderId = "order_USDT_to_{}_{}".format(self.symbol, str(self.counter))            
            msg = "Выставлен ордер на покупку {} по {} USDT".format(self.symbol, self.current_order().price)

        self.h_exec_order(self.current_order())
        self.wait_order_fill = self.current_order()
        self.h_start_timer("{}_compl_timer".format(self.symbol), 5)

        self.h_to_journal(self.symbol, msg)
        ostr = self.print_order_to_string(self.current_order())
        self.h_to_journal(self.symbol, ostr)

        self.fsm.set_state("FSM_ST_WAIT_ORDER_CMPL")

    def fsm_wait_order_compl(self, arg):
        self.h_to_log("{} - FSM state: {} func: {}".format(self.symbol, self.fsm.get_state(), sys._getframe().f_code.co_name))
        try:
            if (self.current_order().newClientOrderId == self.report['c'] and "NEW" == self.report['X']):
                self.reset_order_report()
                self.h_stop_timer("{}_compl_timer".format(self.symbol))

                msg = "Ордер подтвержден Binance, ожидание выполнения"
                self.trade_state["x"] = "order"
                self.trade_state["a"] = "wait"                
                self.trade_state["m"] = msg

                self.h_send_to_web(json.dumps(self.trade_state))
                self.h_to_journal(self.symbol, msg)
                
                self.fsm.set_state("FSM_ST_WAIT_ORDER_FILL")                

        except KeyError:
            self.h_to_log("Binance report: Ошибка ключа 'c' или 'X'")

    def fsm_wait_order_fill(self, arg):
        self.h_to_log("{} - FSM state: {} func: {}".format(self.symbol, self.fsm.get_state(), sys._getframe().f_code.co_name))
        if self.current_order().newClientOrderId == self.report['c'] and "FILLED" == self.report['X']:
            self.h_stop_timer("{}_compl_timer".format(self.symbol))
            self.wait_order_fill = Order()
            msg = "Ордер выполнен Binance"
            self.trade_state["a"] = "ready"
            self.trade_state["m"] = msg            

            self.h_send_to_web(json.dumps(self.trade_state))
            self.h_to_journal(self.symbol, msg)
            self.h_to_log("symbol {} цикл {} шаг {} ".format(self.symbol, self.counter, self.step))

            if self.step == 0:                
                self.step = 1                
            elif self.step == 1:
                self.step = 0
                self.counter = self.counter + 1

            cmsn_asset = self.report['N']

            self.h_to_log("Commission {}/{}".format(cmsn_asset, self.report['n']))            

            if "USDT" == cmsn_asset:
                cmsn_symb = float(self.report['n']) / float(self.report['p'])
                self.orders[0].quantity = self.orders[self.step].quantity - cmsn_symb
                self.orders[0].quantity = self.round_at_step(self.orders[0].quantity, self.stepSize)
                self.orders[1].quantity = self.orders[0].quantity
                self.send_orders_change("Комиссия в USDT, количество в ордере уменьшено")
                self.h_to_log("Correction order {}: new quantity {}".format(self.orders[self.step].newClientOrderId, self.orders[self.step].quantity))
            elif "BNB" != cmsn_asset:
                self.orders[0].quantity = self.orders[self.step].quantity - float(self.report['n'])
                self.orders[0].quantity = self.round_at_step(self.orders[0].quantity, self.stepSize)
                self.orders[1].quantity = self.orders[0].quantity
                self.send_orders_change("Комиссия в {}, количество в ордере уменьшено".format(cmsn_asset))
                self.h_to_log("Correction order {}: new quantity {}".format(self.orders[self.step].newClientOrderId, self.orders[self.step].quantity))

            self.wait_price = True
            self.fsm.set_state("FSM_ST_WAIT_PRICE")

    def fsm_wait_order_del(self, arg):
        self.h_to_log("{} - FSM state: {} func: {}".format(self.symbol, self.fsm.get_state(), sys._getframe().f_code.co_name))
        if "CANCELED" == self.report['X']:            
            msg = "Ордер {} отменен".format(self.wait_order_fill.newClientOrderId)
            self.h_stop_timer("{}_compl_timer".format(self.symbol))
            self.wait_order_fill = Order()
            self.h_to_journal(self.symbol, msg)
            
            self.trade_state["x"] = "order"
            self.trade_state["a"] = "stop"
            self.trade_state["m"] = msg
            self.h_send_to_web(json.dumps(self.trade_state))
            
            if True == self.change_mode:
                self.wait_price = True
                self.fsm.set_state("FSM_ST_WAIT_PRICE")
                self.change_mode = False                
            else:            
                self.fsm.set_state("FSM_ST_RESUME")
                self.fsm.run(None)

    def fsm_resume(self, arg):
        self.h_to_log("{} - FSM state: {} func: {}".format(self.symbol, self.fsm.get_state(), sys._getframe().f_code.co_name))        
        self.h_stop_timer("{}_compl_timer".format(self.symbol))
        self.reset()
        self.running = False        
        msg = "Торги {} остановлены".format(self.symbol)        
        self.trade_state["x"] = "script"
        self.trade_state["a"] = "stop"
        self.trade_state["m"] = msg
        self.h_send_to_web(json.dumps(self.trade_state))
        
        self.h_to_alert_channel(msg)
        self.h_to_journal(self.symbol, msg)
            
        
## Strategy ===============================================================================================

class Strategy(StrategyInstance):
    def __init__(self, server):        
        StrategyInstance.__init__(self, server)
        self.account = Account()
        self.symbol_data = {}
        self.report = json.loads(""" {"X" : "EMPTY", "c" : "no_order"} """)
       
    def eval(self):
       self.to_log("func: {}".format(sys._getframe().f_code.co_name))

       self.symbol_data['BTC'].on_test()
       return

       str = ""
       for s in self.symbol_data:
           str = "FSM state: {}\n".format(self.symbol_data[s].fsm.get_state())
           str += "step {}\n".format(self.symbol_data[s].step)
           str += "triggers: {}\n".format(self.symbol_data[s].trigger)
           str += "orders price {} {}\n".format(self.symbol_data[s].orders[0].price, self.symbol_data[s].orders[1].price)
           str += "orders quantity {} {}\n".format(self.symbol_data[s].orders[0].quantity, self.symbol_data[s].orders[1].quantity)
           str += "\n"

           self.to_log("State symbol {}\n{}:".format(s, str))

    def on_symbols_list(self, list):
        self.to_log("func: {}".format(sys._getframe().f_code.co_name))

        for s in list:
            self.symbol_data[s] = SymbolData(s)
            self.symbol_data[s].h_to_log = self.to_log
            self.symbol_data[s].h_to_journal = self.to_db_journal
            self.symbol_data[s].h_start_timer = self.start_timer
            self.symbol_data[s].h_stop_timer = self.stop_timer
            self.symbol_data[s].h_exec_order = self.exec_order
            self.symbol_data[s].h_delete_order = self.delete_order
            self.symbol_data[s].h_to_alert_channel = self.to_alert_channel
            self.symbol_data[s].h_send_to_web = self.send_to_web            

            ##self.symbol_data[s].h_create_timer = self.create_timer
            self.create_timer("{}_compl_timer".format(s), self.symbol_data[s].on_order_complete_timeout)

    def on_trade_data(self, data):
        self.to_log("func: {}".format(sys._getframe().f_code.co_name))
        json_in_data = {}
        json_out_data = {}
        json_out_data["e"] = "tradeData"
        
        try:
            json_in_data = json.loads(data)
        except json.JSONDecodeError as err:            
            json_out_data["c"] = "001"
            json_out_data["m"] = "Ошибка формата JSON торговых данных"
            self.to_log("Trade data JSON parse error: {}".format(err))
            return

        self.to_log(json.dumps(json_in_data))

        json_out_data["e"] = "tradeData"
        json_out_data["a"] = json_in_data["a"]

        symbol = json_in_data["s"]

        if not symbol in self.symbol_data:    
            json_out_data["c"] = "002"
            json_out_data["m"] = "Валюта {} не поддерживается".format(json_in_data["s"])
            self.send_trade_state(json.dumps(json_out_data))
            return;

        self.symbol_data[symbol].on_trade_data(json_in_data)


    def on_get_trade_state(self, symbol):
        self.to_log("func: {} symbol {}".format(sys._getframe().f_code.co_name, symbol))
        if symbol in self.symbol_data:
            self.send_to_web(json.dumps(self.symbol_data[symbol].trade_state))

    def to_db_journal(self, symbol, msg, alw = False):
        self.to_journal(symbol + '@' + msg)

    def on_account(self, acc):
        self.to_log("func: {}".format(sys._getframe().f_code.co_name))
        
        try:
            jacc = json.loads(acc)
        except json.JSONDecodeError as err:
            self.to_log("Ошибка JSON формата аккаунта REST_API")
            return
            
        self.to_log("Формат аккаунта REST_API")

        try:        
            balances = jacc["balances"]

            for symbol in balances:
                s = symbol['asset']

                if s in self.symbol_data:
                    self.symbol_data[s].t_commission = float(jacc["takerCommission"])/100
                    self.symbol_data[s].m_commission = float(jacc["makerCommission"])/100
                    self.symbol_data[s].balance.free = float(symbol['free'])
                    self.symbol_data[s].balance.locked = float(symbol['locked'])
                    self.to_log("Balance {}:\nfree {}locked {}\n".format(s,
                                                                         self.symbol_data[s].balance.free,
                                                                         self.symbol_data[s].balance.locked))
                elif s == "USDT":
                    for smb in self.symbol_data:
                        self.symbol_data[smb].balance_usdt.free = float(symbol['free'])
                        self.symbol_data[smb].balance_usdt.locked = float(symbol['locked'])

                    self.to_log("Balance : {} free {} / locked {}".format(s,
                                                                          float(symbol['free']),
                                                                          float(symbol['locked'])))

        except KeyError:
            self.to_log("Ошибка ключа аккаунта REST_API")

    def on_balance(self, bal):
        self.to_log("func: {}".format(sys._getframe().f_code.co_name))        
        
        try:
            jbal = json.loads(bal)
        except json.JSONDecodeError as err:
            self.to_log("Ошибка JSON формата аккаунта USER_DATA")
            return
            
        self.to_log("Формат аккаунта USER_DATA")

        try:        
            balances = jbal["B"]

            for symbol in balances:
                s = symbol['a']

                if s in self.symbol_data:
                    self.symbol_data[s].t_commission = float(jbal["t"])/100
                    self.symbol_data[s].m_commission = float(jbal["m"])/100
                    self.symbol_data[s].balance.free = float(symbol['f'])
                    self.symbol_data[s].balance.locked = float(symbol['l'])
                    self.to_log("Balance : {} free {} / locked {}".format(s,
                                                                          self.symbol_data[s].balance.free,
                                                                          self.symbol_data[s].balance.locked))
                elif s == "USDT":
                    for smb in self.symbol_data:
                        self.symbol_data[smb].balance_usdt.free = float(symbol['f'])
                        self.symbol_data[smb].balance_usdt.locked = float(symbol['l'])

                    self.to_log("Balance : {} free {} / locked {}".format(s,
                                                                          float(symbol['f']),
                                                                          float(symbol['l'])))
    
        except KeyError:
            self.to_log("Ошибка ключа аккаунта USER_DATA")

    def on_exchange_info(self, info):
        self.to_log("func: {}".format(sys._getframe().f_code.co_name))

        try:
            json_info = json.loads(info)
        except json.JSONDecodeError as err:
            self.to_log("Ошибка JSON формата EXCHANGE INFO")
            return

        symbols = json_info["symbols"]

        for symbol in symbols:
            if "USDT" != symbol['quoteAsset']:
                continue

            s = symbol['baseAsset']

            if s in self.symbol_data:
                self.symbol_data[s].on_exchange_info(symbol)

                    
    def on_wss_data(self, data):
        wss_data = json.loads(data)
        self.to_log("WSS DATA: %s" % data)

    def on_order_report(self, report):
        self.to_log("func: {}".format(sys._getframe().f_code.co_name))
        self.to_log("ORDER EXEC REPORT: {}".format(report))
        try:
            jreport = json.loads(report)
            try:
                symbol = jreport['s'].replace("USDT", '')
                if symbol in self.symbol_data:
                    self.symbol_data[symbol].on_order_report(jreport)
            except KeyError:
                try:
                    berror = {}
                    berror['e'] = "binanceError"                    
                    berror['m'] = jreport['msg']
                    berror['c'] = jreport['code']
                    self.h_to_alert_channel("Ошибка Binance: {}".format(msg))
                    self.send_to_web(json.dumps(berror))
                except KeyError:
                    self.to_log("Непонятный REPORT от Binance")

        except json.JSONDecodeError as err:
            self.to_log("Ошибка JSON формата EXEC REPORT")

    def on_price(self, data):
        #self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        try:
            pdata = json.loads(data)
            print("on_price: {} {}".format(pdata['symbol'], pdata['price']))
            symbol = pdata['symbol'].replace("USDT", '')
            if symbol in self.symbol_data:
                self.symbol_data[symbol].on_price(pdata['price'])
        except json.JSONDecodeError as err:
            self.to_log("Ошибка JSON формата PRICE")

    def on_trade_run(self, data):
        self.to_log("func: {}".format(sys._getframe().f_code.co_name))
        try:
            rjdata = json.loads(data)
            symbol = rjdata['s']
            if symbol in self.symbol_data:
                self.symbol_data[symbol].on_trade_run(rjdata)
            
        except json.JSONDecodeError as err:
            self.to_log("Ошибка JSON формата RUN TRADE")

    def get_trade_steps(self, symbol):
        self.to_log("func: {}".format(sys._getframe().f_code.co_name))
        if symbol in self.symbol_data:
            return self.symbol_data[symbol].get_trade_steps()
        else:
            return []            

    def get_exchange_info(self):
        self.to_log("func: {}".format(sys._getframe().f_code.co_name))
        filters = []

        for s, sdata in self.symbol_data.items():
            filter = {}
            filter['s'] = sdata.symbol
            filter['pb'] = sdata.quotePrecision
            filter['pa'] = sdata.quoteAssetPrecision
            filter['ts'] = sdata.tickSize
            filter['ss'] = sdata.stepSize
            filter['mu'] = sdata.multiplierUp
            filter['mn'] = sdata.minNotional
            filter['md'] = sdata.multiplierDown
            filters.append(filter)

        fdata = {'e': 'getFilters'}
        fdata['f'] = filters
        self.send_to_web(json.dumps(fdata))
