//
// Created by WMJ on 2022/9/7.
//

#ifndef NPP_LOGGER_H
#define NPP_LOGGER_H

#include <string>
#include <sstream>
#include <fmt/format.h>

const char *file_name(const char *path);

namespace NPP {
    class Logger {
       std::string prefix;
       bool time;
       std::ostream& out;
    public:
        void setPrefix(const std::string& s);
        Logger(std::string name, bool showTime);
        void print(std::string_view);

        template<typename ...Args>
        void print(Args... partial) {
            std::stringstream ss;
            (ss << ... << partial);
            print(ss.view());
        }

        template<typename T, typename ...Args>
        std::string format(const T& t, Args... args) {
            return fmt::format(fmt::runtime(t), args...);
        }

        template<typename ...Args>
        void info(Args... args) {
            print("[INFO] ", format(args...));
        }

        template<typename ...Args>
        void warning(Args... args) {
            print("[WARNING] ", format(args...));
        }

        template<typename ...Args>
        void error(Args... args) {
            print("[ERROR] ", format(args...));
        }

        template<typename ...Args>
        void debug(const char*file, int line, Args... args) {
            if (file) {
                print("[DEBUG] ", fmt::format("{}:{} ", file_name(file), line), format(args...));
            } else {
                print("[DEBUG] ", format(args...));
            }
        }
    };

    class Log {
    public:
        static Logger global;
    };
} // NPP

#define logi(...) NPP::Log::global.info(__VA_ARGS__)
#define logw(...) NPP::Log::global.warning(__VA_ARGS__)
#define loge(...) NPP::Log::global.error(__VA_ARGS__)

#ifdef DEBUG
#define logd(...) NPP::Log::global.debug(__FILE__, __LINE__, __VA_ARGS__)
#else
#define logd(...) do {} while(0)
#endif
#define tick()    logd("tick")

#endif //NPP_LOGGER_H
