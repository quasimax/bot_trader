#ifndef UTILS_H
#define UTILS_H

#include <boost/chrono/include.hpp>
#include <boost/lexical_cast.hpp>
#include <iomanip>
#include <ctime>

class utils
{
public:
    static inline uint64_t curr_time_milliseconds()
    {
        using namespace std::chrono;
        return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    }

    static std::string double_to_string(double val, int prec)
    {
        std::ostringstream stream;
        stream << std::fixed;
        stream << std::setprecision(prec);
        stream << val;
        return stream.str();
    }

    static std::string local_time_from_ms(const uint64_t utime)
    {
        time_t sct = utime/1000;
        std::tm tm = *std::localtime(&sct);
        const std::string tz = "TZ=Europe/Moscow";
        putenv(const_cast<char*>(tz.data()));
        return boost::lexical_cast<std::string>(std::put_time(&tm, "%c %Z"));
    }
};

#endif // UTILS_H
