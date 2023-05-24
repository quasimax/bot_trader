#ifndef GENERAL_H
#define GENERAL_H

//#define LOCAL 1
#define REDIRECT_OUT 1
#define REDIRECT_ERR 1
#define THREADS_NUM  5

#ifndef LOCAL
#define DEF_CONF_DIR    "/usr/local/etc/"
#define DEF_LOG_DIR     "/var/log/"
#define DEF_DB_DIR      "/usr/local/etc/"
#define DEF_LOCK_DIR    "/var/run/"
#else
#define DEF_DB_DIR      "/home/opelsin/.gbot/"
#define DEF_CONF_DIR    "/usr/local/etc/"
#define DEF_LOG_DIR     "/home/opelsin/.gbot/"
#endif

enum uinterface {IntfTlg, IntfWeb};

#endif // GENERAL_H
