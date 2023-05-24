#include "py_wrapper.h"

#include <boost/python/class.hpp>

BOOST_PYTHON_MODULE(StrategyFramework)
{
    python::enum_<BncOrder::Side>("Side")
        .value("BUY", BncOrder::BUY)
        .value("SELL", BncOrder::SELL)
        ;

    python::enum_<BncOrder::Type>("Type")
        .value("LIMIT", BncOrder::LIMIT)
        .value("MARKET", BncOrder::MARKET)
        .value("STOP_LOSS", BncOrder::STOP_LOSS)
        .value("STOP_LOSS_LIMIT", BncOrder::STOP_LOSS_LIMIT)
        .value("TAKE_PROFIT", BncOrder::TAKE_PROFIT)
        .value("TAKE_PROFIT_LIMIT", BncOrder::TAKE_PROFIT_LIMIT)
        .value("LIMIT_MAKER", BncOrder::LIMIT_MAKER)
        ;

    python::enum_<BncOrder::TimeInForce>("TimeInForce")
        .value("FOK", BncOrder::FOK)
        .value("GTC", BncOrder::GTC)
        .value("IOC", BncOrder::IOC)
        ;


    python::class_<bnc_order_t>("Order")
        .def_readwrite("symbol", &bnc_order_t::symbol)
        .def_readwrite("newClientOrderId", &bnc_order_t::newClientOrderId)
        .def_readwrite("type", &bnc_order_t::type)
        .def_readwrite("side", &bnc_order_t::side)
        .def_readwrite("quantity", &bnc_order_t::quantity)
        .def_readwrite("timeInForce", &bnc_order_t::timeInForce)
        .def_readwrite("quoteOrderQty", &bnc_order_t::quoteOrderQty)
        .def_readwrite("price", &bnc_order_t::price)
        .def_readwrite("stopPrice", &bnc_order_t::stopPrice)
        .def_readwrite("icebergQty", &bnc_order_t::icebergQty)
        ;

    python::enum_<OrderAction>("OrderAction")
        .value("Execute", OrderAction::OA_Execute)
        .value("View", OrderAction::OA_View)
        .value("Correction", OrderAction::OA_Correction)
        ;

    python::enum_<TlgKeyboard::Side>("Orientation")
        .value("Vertical", TlgKeyboard::Vertical)
        .value("Horizontal", TlgKeyboard::Horizontal)
        ;

    //PyEval_InitThreads();

    python::class_<StrategyServer>("StrategyServer")
        ;

    python::class_<PyStrategyInstance, boost::noncopyable>("StrategyInstance", python::init<StrategyServer&>())
        .def("exec_order", &StrategyInstance::execOrder)        
        .def("delete_order", &StrategyInstance::deleteOrder)
        .def("delete_all_orders", &StrategyInstance::deleteAllOrders)
        .def("send_message", &StrategyInstance::sendMessage, (python::arg("msg"), python::arg("alw") = false))
        .def("to_log", &StrategyInstance::toLog)
        .def("request_value", &StrategyInstance::requestValue)
        .def("request_command", &StrategyInstance::requestCommand, (python::arg("msg"), python::arg("keys"), python::arg("orientation") = TlgKeyboard::Vertical))
        .def("get_price", &StrategyInstance::getPrice)
        .def("add_stream", &StrategyInstance::addStream)
        .def("del_stream", &StrategyInstance::addStream)
        .def("send_to_web", &StrategyInstance::toFront)
        .def("to_journal", &StrategyInstance::toJournal)
        .def("to_alert_channel", &StrategyInstance::toAlertChannel)

        .def("create_timer", &StrategyInstance::createTimer, (python::arg("name"), python::arg("function")))
        .def("start_timer", &StrategyInstance::startTimer, (python::arg("interval"), python::arg("recurring") = false))
        .def("stop_timer", &StrategyInstance::stopTimer)
        .def("del_timer", &StrategyInstance::delTimer)
        ;

}

python::object import(const std::string& module, const std::string& path, python::object& globals)
{
    python::dict locals;
    locals["module_name"] = module;
    locals["path"]        = path;

    python::exec("import imp\n"
             "new_module = imp.load_module(module_name, open(path), path, ('py', 'U', imp.PY_SOURCE))\n",
             globals,
             locals);

    return locals["new_module"];
}

bool init_strategy_interface(ss_callbacks_t& callbacks, const char* sprategy_path, python::object& strategy)
{
    try
    {
        StrategyServer strategy_server;
        strategy_server.set_callbacks(callbacks);

        PyImport_AppendInittab("StrategyFramework", &PyInit_StrategyFramework);
        Py_Initialize();

        python::object main     = python::import("__main__");
        python::object globals  = main.attr("__dict__");
        python::object module   = import("strategy", sprategy_path, globals);
        python::object Strategy = module.attr("Strategy");
        strategy = Strategy(strategy_server);

        return true;
    }
    catch(const python::error_already_set&)
    {
        PyErr_Print();
        return false;
    }
}
