//
// Created by WMJ on 2022/9/3.
//

#ifndef NPP_EXCEPTION_H
#define NPP_EXCEPTION_H

#include <stdexcept>
#include <iostream>
#include <bits/types/siginfo_t.h>
#include <fmt/format.h>

namespace NPP {
constexpr size_t dump_size = 128;

class Exception: public std::exception {
    void *array[dump_size]{};
    int size;
    std::string info;
public:
    Exception();
    explicit Exception(const char* info);
    void print_backtrace(std::ostream &out=std::cerr);

    static void terminate_handler();
    static void signal_handler(int signum, siginfo_t *siginfo, void *context);
    static void initExceptionHandler();
};

class SignalException: public Exception {
public:
    explicit SignalException(int signum);
};

class RangeException: public Exception {
public:
    explicit RangeException(const char* s): Exception(s) {}
};

class RuntimeException: public Exception {
public:
    explicit RuntimeException(const char *s): Exception(s) {}
};

} // NPP

#define DEFINE_EXCEPTION(name) struct name: public NPP::Exception {explicit name(const char* s): Exception(s) {}}

#endif //NPP_EXCEPTION_H
