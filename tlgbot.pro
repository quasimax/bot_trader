TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
CONFIG += std=++14
CONFIG += static

TARGET = bibot

#Debug: DEFINES += LOCAL=1

LIBS += -L/usr/local/lib/ -lboost_system -lboost_program_options -lboost_chrono -lboost_thread -lboost_filesystem
LIBS += -L/usr/local/lib/ -lssl -lcrypto -lpthread -lsqlite3 -llog4cplus

INCLUDEPATH += /usr/include/python3.6m
LIBS += -lboost_python3 -lboost_system -lpython3.6m

OBJECTS_DIR = .obj

SOURCES += \
        application.cpp \
        binance.cpp \
        bnc_data_types.cpp \
        https_client.cpp \
        logger.cpp \
        main.cpp \
        order.cpp \
        py_wrapper.cpp \
        statistic.cpp \
        strategyserver.cpp \
        telegram.cpp \
        webhook_server.cpp \
        wss_client.cpp \
        wss_server.cpp

HEADERS += \
    application.h \
    binance.h \
    bnc_data_types.h \
    database.h \
    events.h \
    general.h \
    html_messages.h \
    https_client.h \
    logger.h \
    order.h \
    py_wrapper.h \
    sock4a.hpp \
    statistic.h \
    strategyserver.h \
    telegram.h \
    timer.h \
    tkeyboard.h \
    utils.h \
    webhook_server.h \
    wss_client.h \
    wss_server.h

DISTFILES += \
    CMakeLists.txt \
    distrib/bibot-deb/DEBIAN/conffiles \
    distrib/bibot-deb/DEBIAN/control \
    distrib/bibot-deb/DEBIAN/init.d \
    distrib/bibot-deb/usr/local/bin/bibot \
    distrib/bibot-deb/usr/local/bin/mstrategy.py \
    distrib/bibot-deb/usr/local/bin/strategy.py \
    distrib/bibot-deb/usr/local/etc/bibot.conf \
    distrib/bibot-deb/usr/local/etc/bibot.sqlite \
    distrib/build-deb.sh \
    mstrategy.py \
    scrdbg/mstrategy.py \
    scrdbg/strategy_dbg.py \
    strategy_sceleton.py
