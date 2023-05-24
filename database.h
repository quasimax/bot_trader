#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <map>
#include <boost/variant.hpp>
#include <boost/lexical_cast.hpp>
#include <sqlite3.h>
#include "logger.h"

class database
{
    static int callback(void* data, int argc, char** argv, char** azColName)
    {
        query_result_t* query_result_list = static_cast<query_result_t*>(data);
        query_result_list->push_back(values_map_t());

        for(int i = 0; i < argc; i++)
            query_result_list->back().insert(make_pair(azColName[i], std::string(argv[i])));

        return 0;
    }

public:
    database(){}

    bool open(const std::string& fname)
    {
        int rc = sqlite3_open(fname.c_str(), &m_db);

        if(rc)
            mlogger::error("Ошибка доступа к БД: %s", sqlite3_errmsg(m_db));
        else
            mlogger::info("Доступ к БД - Ок");

        return !rc;
    }

    inline void close(){sqlite3_close(m_db);}

    bool exec(const std::string& qstr)
    {
        m_query_result.clear();
        m_next_query = true;

        int rc = sqlite3_exec(m_db,
                              qstr.c_str(),
                              callback,
                              static_cast<void*>(&m_query_result), &m_db_err_msg);

        rit = m_query_result.end();

        if(rc != SQLITE_OK)
        {
            mlogger::error("Ошибка запроса к БД: %s", m_db_err_msg);
            sqlite3_free(m_db_err_msg);
        }

        return !rc;
    }

    template<typename T>
    const T get_value(const std::string& name, const T& def_val = T(), bool* ok = nullptr)
    {
        if(nullptr != ok)*ok = true;

        if(m_query_result.end() == rit)
        {
            if(nullptr != ok)*ok = false;
            return def_val;
        }

        auto it = m_query_result.front().find(name);
        if(m_query_result.front().end() == it)
        {
            if(nullptr != ok)*ok = false;
            return T();
        }

        try
        {
            return boost::lexical_cast<T>(it->second);
        }
        catch(boost::bad_lexical_cast &e)
        {
            mlogger::error("DB value cast: %s", e.what());
        }

        if(nullptr != ok)*ok = false;
        return def_val;
    }

    bool next()
    {
        if(rit != m_query_result.end())
            m_query_result.erase(rit);

        rit = m_query_result.begin();
        return rit != m_query_result.end();
    }

private:
    sqlite3* m_db;
    char* m_db_err_msg;
    bool m_next_query{false};

    typedef std::map<std::string, std::string> values_map_t;
    typedef std::list<values_map_t> query_result_t;

    query_result_t m_query_result;
    query_result_t::iterator rit{m_query_result.end()};


};

#endif // DATABASE_H
