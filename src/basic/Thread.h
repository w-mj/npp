//
// Created by WMJ on 2022/9/4.
//

#ifndef NPP_THREAD_H
#define NPP_THREAD_H

#include <thread>

namespace NPP {
    class Thread {
        std::jthread thread;
        bool running = false;
        bool finished = false;
    public:
        virtual void run() = 0;
        void start();
        void join();
        void detach();

        [[nodiscard]] bool isRunning() const;
        [[nodiscard]] bool isFinished() const;
    };
}


#endif //NPP_THREAD_H
