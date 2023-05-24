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

#================= FSM =====================#

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

#================= DEBUG FUNCTIONS AND STRUCTS =====================#
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
            self.symbol = "BTCUSDT"
            self.newClientOrderId = ""

    class Account():
        def __init__(self):
            self.maker_commission = 0.00
            self.taker_commission = 0.00
            self.free_usdt = 0.00
            self.locked_usdt = 0.00
            self.free_btc = 0.00
            self.locked_btc = 0.00


    class StrategyInstance():
        def __init__(self, server):
            self.server = server
            self.timer = threading.Timer(0, self.stop_timer)

        def exec_order(self, order):
            print("DEBUG func: {}".format(sys._getframe().f_code.co_name))

        def correct_order(self, order, autocancel):
            print("DEBUG func: {}".format(sys._getframe().f_code.co_name))

        def delete_order(self, oname):
            print("DEBUG func: {}".format(sys._getframe().f_code.co_name))

        def delete_all_orders(self):
            print("DEBUG func: {}".format(sys._getframe().f_code.co_name))            

        def send_message(self, msg):
            print("DEBUG func: {}".format(sys._getframe().f_code.co_name))
            print("TLG MSG: %s", msg)

        def request_value(self, msg):
            print("DEBUG func: {}".format(sys._getframe().f_code.co_name))
            print("TLG USER VAL: %s", msg)

        def create_timer(self, interval, func, recurring = False):
            print("DEBUG func: {}".format(sys._getframe().f_code.co_name))
            self.timer = threading.Timer(interval, func)
            self.timer.start()
            
        def stop_timer(self):
            print("DEBUG func: {}".format(sys._getframe().f_code.co_name))
            self.timer.cancel()

#================= STRATEGY CLASS =====================#

class Strategy(StrategyInstance):
    def __init__(self, server):        
        StrategyInstance.__init__(self, server)
        
        self.fsm = StateMachine()
        self.fsm.add_state("FSM_ST_READY", self.fsm_ready)
        self.fsm.add_state("FSM_ST_START", self.fsm_start)

        self.fsm.add_state("FSM_ST_RESUME", self.fsm_resume)
        self.fsm.add_state("FSM_ST_STOP", None, end_state = 1)
        self.fsm.set_start("FSM_ST_START")

        self.alert_dict = {}
        self.kline_low_list = [float(0.00),float(0.00)]
        self.line = float(0.00)
        self.kline_open = float(0.00)
        self.kline_close = float(0.00)

        self.fsm.set_state("FSM_ST_READY")

    def reset_all(self):
        self.stop_timer()
        self.reset_correct_mode()
        self.kline_low_list = [float(0.00),float(0.00)]
        self.line = float(0.00)
        self.kline_open = float(0.00)
        self.kline_close = float(0.00)

    # сигнал от TreadingView
    def on_alert(self, name, body):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        tmp_dict = dict(x.split("=") for x in body.split("\n"))

#        if tmp_dict["label"] == "MACD" :
#            tmp_dict["Итог: "] = "Покупать" if float(tmp_dict["MACD"]) > 0.00 else "Продавать"

        self.alert_dict[tmp_dict["label"]] = tmp_dict


    def dict_to_string(self, dict):
        str = ""
        for key, value in dict.items():
            if "label" == key:
                continue
            if "title" == key:
                key = "Индикатор:"

            str += key + " " + value + "\n"

        return str

#================= CALL FROM BOT FUNCTIONS =====================#

    # тестовая функция
    def eval(self):
       self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
       #self.fsm.run(None)

    # запуск скрипта
    def on_start(self):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        if "FSM_ST_READY" != self.fsm.get_state() and "FSM_ST_STOP" != self.fsm.get_state():
            self.send_message("FSM: Невозможно выполнить: скрипт работает")
        else:
            self.fsm.set_state("FSM_ST_START")
            self.fsm.run(None)

    # остановка скрипта
    def on_stop(self):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        self.send_message("FSM: Скрипт остановлен пользователем")
        self.stop_timer()
        self.delete_all_orders()
        self.fsm.set_state("FSM_ST_RESUME")
        self.fsm.run(None)

    # вывод пользователю данных индикаторов от TreadingView
    def on_indicators(self):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        for key, value in self.alert_dict.items():
            msg = self.dict_to_string(value)
            self.send_message(msg)

    # пришли данные аккаунта
    def on_account(self, acc):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        self.account = acc

    # пришли данные свечи
    def on_kline(self, kl_open, kl_close):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        if len(self.kline_low_list) < 2:
            self.kline_low_list.append(min(kl_open, kl_close))
        else:
            self.kline_low_list.pop()
            self.kline_low_list.insert(0, min(kl_open, kl_close))


    # команда от пользователя
    def on_command(self, answ):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        self.to_log("USER COMMAND: {}".format(answ))

    def on_user_value(self, val):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        self.to_log("USER VALUE: {}".format(answ))

    # ордер после редактирования
    def on_order_update(self, order):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        self.order = order
        self.send_message("FSM: Ордер {} обновлен".format(self.order.newClientOrderId))

    # отчет о статусе ордера от Binance
    def on_order_exc(self, rep):    #pep - json string
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        self.report = json.loads(rep)
        self.to_log("ORDER EXEC REPORT: {}".format(rep))


#================= SERVICE FUNCTIONS =====================#

    def is_cash_in btc():
        return (self.account.free_usdt / self.kline_close) < self.account.free_btc

    def is_line_in_kline():
        return self.line in range(self.kline_close, kline_open)

    def is_market_fall(self):
        return self.kline_low_list[0] < self.kline_low_list[1]

#================= FSM FUNCTIONS =====================#

    def fsm_ready(self, arg):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        self.fsm.run(None)

    def fsm_start(self, arg):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        self.send_message("FSM: Скрипт ЗАПУЩЕН");

    def fsm_resume(self, arg):
        self.to_log("FSM state: {} func: {}".format(self.fsm.get_state(), sys._getframe().f_code.co_name))
        self.send_message("FSM: Скрипт ОСТАНОВЛЕН");
        self.reset_all()
        self.fsm.set_state("FSM_ST_READY")
        self.fsm.run(None)
        self
