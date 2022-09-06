//
// Created by WMJ on 2022/9/3.
//

#include <execinfo.h>
#include "Exception.h"
#include <iostream>
#include <sstream>
#include <cxxabi.h>

#include <unistd.h>
#include <cstring>
#include <utility>
#include <dlfcn.h>
#include "basic/Utils.h"
#include "fmt/format.h"
#include <signal.h>


#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif


static std::string Addr2LinePrint(void const * const addr, const char *execFile)
{
    char addr2lineCmd[512] = {0};
    sprintf(addr2lineCmd, "addr2line -C -i -f -p -s -a -e %s %p ", execFile, addr);
    return NPP::exec(addr2lineCmd);
}


void parseStrings(char *stackFrameString, char **execFile, char **symbolString, char **offsetString) {
    *execFile = stackFrameString;
    for(char *iteratorPointer = stackFrameString; *iteratorPointer; iteratorPointer++) {
        if(*iteratorPointer == '(') {
            *symbolString = iteratorPointer + 1;
            *iteratorPointer = 0;
        } else if(*iteratorPointer == '+') {
            *offsetString = iteratorPointer + 1;
            *iteratorPointer = 0;
        } else if(*iteratorPointer == ')')
        {
            *iteratorPointer = 0;
        }
    }
}

void* CalculateOffset(char * stackFrameString, char **execFile) {
    void *     objectFile;
    void *     address;
    void *     offset = nullptr;
    char *      symbolString;
    char *      offsetString;
    char *      execString;
    char *      dlErrorSting;
    int        checkSscanf = EOF;
    int        checkDladdr = 0;
    Dl_info    symbolInformation;

    parseStrings(stackFrameString, &execString, &symbolString, &offsetString);

    checkSscanf = sscanf(offsetString, "%p", &offset);

    if(symbolString[0] != '\0') {
        objectFile = dlopen(nullptr, RTLD_LAZY);
        if(!objectFile) {
            dlErrorSting = dlerror();
            (void)write(STDERR_FILENO, dlErrorSting, strlen(dlErrorSting));
        }
        address = dlsym(objectFile, symbolString);
        if(address == nullptr) {
            dlErrorSting = dlerror();
            (void)write(STDERR_FILENO, dlErrorSting, strlen(dlErrorSting));
        }
        checkDladdr = dladdr(address, &symbolInformation);

        if(checkDladdr != 0) {
            offset = reinterpret_cast<void *>(
                    ((uint64_t) symbolInformation.dli_saddr - (uint64_t) symbolInformation.dli_fbase) +
                    (uint64_t) offset);
            dlclose(objectFile);
        }
        else {
            dlErrorSting = dlerror();
            (void)write(STDERR_FILENO, dlErrorSting, strlen(dlErrorSting));
        }
    }
    *execFile = execString;
    return checkSscanf != EOF ? offset : NULL;
}



namespace NPP {
    Exception::Exception(): Exception("no info") {

    }

    Exception::Exception(const char* info): info(info) {
        size = backtrace(array, dump_size);
    }

    void Exception::print_backtrace(std::ostream &out) {
        char **symbols = backtrace_symbols(array, size);
        std::ostringstream oss;

        oss << "Obtained "<< size << " stack frames." << std::endl;
        char *execFile;
        for (int i = 0; i < size; ++i) {
            // oss << symbols[i] << std::endl;
            void *offset = CalculateOffset(symbols[i], &execFile);
            if (offset == nullptr) {
                oss << "Offset cannot be resolved: No offset present?\n";
            } else {
                oss << Addr2LinePrint(offset, execFile);
            }
        }
        free(symbols);
        oss << std::endl;

        out << oss.str();
    }

    void Exception::terminate_handler() {
        std::exception_ptr exp = std::current_exception();
        try {
            if (exp) {
                std::rethrow_exception(exp);
            }
        } catch (Exception& e) {
            int status;
            char *expType = abi::__cxa_demangle(typeid(e).name(), nullptr, nullptr, &status);
            std::cout << (status == 0? expType: typeid(e).name()) << ": " << e.info << '\n';
            e.print_backtrace(std::cout);
        } catch (std::exception& e) {
            std::cout << "Caught exception \"" << e.what() << "\"\n";
        }
        exit(-1);
    }

    void Exception::signal_handler(int signum, siginfo_t *siginfo, void *context) {
        throw SignalException(signum);
    }

    void Exception::initExceptionHandler() {
        std::set_terminate(NPP::Exception::terminate_handler);
        struct sigaction sig_action = {};
        sig_action.sa_sigaction = Exception::signal_handler;
        sigemptyset(&sig_action.sa_mask);


        sig_action.sa_flags = SA_SIGINFO | SA_ONSTACK;

        sigaction(SIGSEGV, &sig_action, nullptr);
        sigaction(SIGFPE,  &sig_action, nullptr);
        sigaction(SIGINT,  &sig_action, nullptr);
        sigaction(SIGILL,  &sig_action, nullptr);
        sigaction(SIGTERM, &sig_action, nullptr);
        sigaction(SIGABRT, &sig_action, nullptr);
    }


    SignalException::SignalException(int signum): Exception(strsignal(signum)) {

    }
} // NPP

