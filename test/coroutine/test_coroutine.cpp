//
// Created by WMJ on 2022/9/7.
//

#include "task/Task.h"
#include "task/SleepAwaiter.h"
#include <chrono>
#include <basic/Logger.h>
#include <basic/Exception.h>
using namespace std;

Task<void> myTask(int id, std::vector<int> sleeps) {
    logi("start task {}", id);
    time_t s = time(nullptr);
    int ss = 0;
    for (int i: sleeps) {
        ss += i;
        logi("{} sleep {}", id, i);
        if (i % 2 == 0) {
            std::this_thread::sleep_for(std::chrono::seconds(i));
        } else {
            co_await std::chrono::seconds(i);
        }
    }
    logi("finish task {} {}s {}s", id, time(nullptr) - s, ss);
    co_return ;
}

Task<void, NoopExecutor> runTask() {
    time_t start_time = time(nullptr);
    Task<void> d[] = {
        myTask(0, {1, 2, 3, 4, 5}),
        myTask(1, { 2, 4, 6, 8, 10 }),
        myTask(2, { 3, 2, 3, 4, 5 }),
        myTask(3, { 1, 2, 1, 4, 5 }),
        myTask(4, { 1, 2, 1, 4, 5 }),
        myTask(5, { 1, 2, 1, 4, 5 }),
        myTask(6, { 1, 2, 1, 4, 5 }),
    };
    logi("All task emitted!");
    for (int i = 0; i < 4; i++) {
        d[i].get_result();
    }
    logi("All task finished! {}s", time(nullptr) - start_time);
    co_return;
}

int main() {
    tick();
    NPP::Exception::initExceptionHandler();
    runTask().get_result();
    return 0;
}