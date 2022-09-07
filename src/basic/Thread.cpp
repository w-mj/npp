//
// Created by WMJ on 2022/9/4.
//

#include "Thread.h"
#include "basic/Exception.h"

void NPP::Thread::start() {
    if (finished) {
        throw RuntimeException("thread is already running");
    }
    std::jthread newThread([this](){
        finished = false;
        running = true;
        this->run();
        running = false;
        finished = true;
    });
    std::swap(thread, newThread);
}

void NPP::Thread::join() {
    if (thread.joinable()) {
        thread.join();
    }
}

void NPP::Thread::detach() {
    thread.detach();
}

bool NPP::Thread::isRunning() const {
    return running;
}

bool NPP::Thread::isFinished() const {
    return finished;
}

NPP::Thread::~Thread() {
    join();
}
