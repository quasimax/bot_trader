#define _POSIX_SOURCE

#include <iostream>
#include <string>
#include <boost/program_options.hpp>
#include <boost/thread/thread.hpp>
#include <boost/filesystem.hpp>

namespace po = boost::program_options;
using namespace std;

#include "application.h"
#include "logger.h"
#include "general.h"

int redirect_stream(const char* app_name, int stream)
{
    std::string fn(app_name);
    fn.append(".daemon.");

    switch(stream)
    {
        case 1:
            fn += "out";
            break;

        case 2:
            fn += "err";
        break;

        default:
            return 0;
    }

    std::string old_fn = fn + ".old";

    if(boost::filesystem::exists(old_fn))
        remove(old_fn.c_str());

    if(boost::filesystem::exists(fn))
        rename(fn.c_str(), old_fn.c_str());

    const int flags = O_WRONLY | O_CREAT | O_APPEND;
    const mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    if(open(fn.c_str(), flags, mode) < 0)
    {
        mlogger::error("Unable to open output file %s: %d", fn, stream);
        return 1;
    }

    if (dup(stream) < 0)
    {
        mlogger::error("Unable to dup output descriptor: %m", stream);
        return 1;
    }

    return 0;
}

int as_daemon(const char* app_name, boost::asio::io_context& ioc)
{
#if ! defined(REDIRECT_OUT) && ! defined(REDIRECT_ERR)
    boost::ignore_unused(app_name);
#endif


    ioc.notify_fork(boost::asio::io_context::fork_prepare);

    if (pid_t pid = fork())
    {
        if (pid > 0)
        {
           exit(0);
        }
        else
        {
          mlogger::error("First fork failed: %m");
          return 1;
        }
    }

    setsid();
    chdir("/");
    umask(0);

    if (pid_t pid = fork())
    {
        if (pid > 0)
        {
            exit(0);
        }
        else
        {
            mlogger::error("Second fork failed: %m");
            return 1;
        }
    }

    close(0);
    close(1);
    close(2);

    if (open("/dev/null", O_RDONLY) < 0)
    {
        mlogger::error("Unable to open /dev/null: %m");
        return 1;
    }

#ifdef REDIRECT_OUT
    redirect_stream(app_name, 1);
#endif

#ifdef REDIRECT_ERR
    redirect_stream(app_name, 2);
#endif

    ioc.notify_fork(boost::asio::io_context::fork_child);

    return 0;
}

int create_pid_file(const char* app_name)
{
#ifdef LOCAL
    boost::ignore_unused(app_name);
    return 0;
#else
    FILE* fp;
    int pid = getpid();
    char fpid[128] = DEF_LOCK_DIR;
    const char* pos = strrchr(app_name, '/');

    if(NULL == pos)
        strcat(fpid, app_name);
    else
        strcat(fpid, pos + 1);

    strcat(fpid, ".pid");

    fp = fopen(fpid, "w+");
    if(NULL == fp)
    {
        return 1;
    }
    fprintf(fp, "%d", pid);
    fclose(fp);

    return 0;
#endif
}

int main(int argc, const char* argv[])
{
    int ret;

    app_init_t app_data;
    app_data.conf_file_path = string(DEF_CONF_DIR) + "bibot.conf";
    app_data.log_file_path = string(DEF_LOG_DIR) + "bibot.log";

    po::options_description opt_desc{"Разрешенные опции"};
    po::variables_map vm; // контейнер для сохранения выбранных опций программы

    boost::asio::io_context ioc{THREADS_NUM};
    boost::thread_group threadgroup;



    asio::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&](boost::system::error_code, int)
                       {
                           ioc.stop();
                       });



    opt_desc.add_options()
            ("help", "вызов справки")
            ("daemon,d", "запуск в режиме демона")
            ("log,l",  po::value<fs::path>(&app_data.log_file_path)->composing(), "log файл")
            ("conf,c",  po::value<fs::path>(&app_data.conf_file_path)->composing(), "файл конфигурации");


    po::store(po::parse_command_line(argc, argv, opt_desc), vm);  // парсер переданных аргументов
    po::notify(vm); // записываем аргументы в переменные

    if (vm.count("help"))
    {
        //выводим описание меню
        std::cout << opt_desc << std::endl;
    }

    if (vm.count("daemon"))
    {
        ret = as_daemon(argv[0], ioc);
        if(0 != ret)
            return ret;

        ret = create_pid_file(argv[0]);
    }

    if (vm.count("log"))
        app_data.log_file_path = vm["log"].as<fs::path>();

    if (vm.count("conf"))
        app_data.conf_file_path = vm["conf"].as<fs::path>();

    mlogger::init();
    mlogger::init_common_log(app_data.log_file_path.c_str());

    mlogger::info("Bot backend запущен..");

    Application* app = new Application(app_data, ioc);
    ret = app->init();
    if(0 == ret)
    {
        for(auto i = THREADS_NUM - 1; i > 0; --i)
        {
            threadgroup.create_thread(boost::bind(&asio::io_context::run, &ioc));
        }
        ioc.run();
        threadgroup.join_all();
    }
    delete app;

    mlogger::info("Bot backend остановлен..");
    mlogger::deinit();

    return ret;
}
