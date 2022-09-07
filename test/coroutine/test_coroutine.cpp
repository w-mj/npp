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
    for (int i: sleeps) {
        logi("{} sleep {}", id, i);
        std::this_thread::sleep_for(std::chrono::seconds(i));
        // co_await SleepAwaiter(std::chrono::seconds(i));
    }
    logi("finish task {}", id);
    co_return ;
}

Task<void, NoopExecutor> runTask() {
    time_t start_time = time(nullptr);
    Task<void> d[] = {
        myTask(0, {1, 2, 3, 4, 5}),
        myTask(1, { 2, 4, 6, 8, 10 }),
        myTask(2, { 3, 2, 3, 4, 5 }),
        myTask(3, { 1, 2, 1, 4, 5 }),
    };
    logi("All task emitted!");
    for (int i = 0; i < 4; i++) {
        d[i].get_result();
    }
    logi("All task finished! {} ", time(nullptr) - start_time);
    co_return;
}

int main() {
    NPP::Exception::initExceptionHandler();
    runTask();
    return 0;
}