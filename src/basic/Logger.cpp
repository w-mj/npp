//
// Created by WMJ on 2022/9/7.
//

#include "Logger.h"
#include <iostream>

#include <ctime>
#include <string>


static std::string get_time_str() {
    std::string stime;
    std::stringstream strtime;
    std::time_t currenttime = std::time(nullptr);
    char tAll[255];
    std::strftime(tAll, sizeof(tAll), "%H:%M:%S", std::localtime(&currenttime));
    strtime << tAll;
    return strtime.str();
}

namespace NPP {
    void Logger::print(std::string_view sv) {
        static std::mutex write_lock;
        std::lock_guard<std::mutex> guard(write_lock);
        if (!name.empty()) {
            out << name;
        }
        if (time) {
            out << '[' << get_time_str() << ']';
        }
        out << sv << std::endl;
    }

    Logger::Logger(std::string name, bool showTime): name(std::move(name)), time(showTime), out(std::cout) {
    }


} // NPP

NPP::Logger NPP::Log::global("", true);