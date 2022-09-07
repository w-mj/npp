//
// Created by WMJ on 2022/9/7.
//

#include "ElasticExecutor.h"
#include "basic/Thread.h"
#include "tbb/concurrent_queue.h"
#include "basic/Logger.h"
#include <chrono>
#include <list>

using TaskType = std::function<void()>;
constexpr float expand_threshold = 0.8;
constexpr float expire_threshold = 0.2;
constexpr int occupied_rate_sample_period = 2000; // 2s

static uint64_t get_time() {
    timeval t{};
    gettimeofday(&t, nullptr);
    return t.tv_sec * 1000 + t.tv_usec / 1000;
}


class NPP::ElasticExecutorImpl: public NPP::Thread {
    class WorkerThread: public NPP::Thread {
        NPP::ElasticExecutorImpl& impl;
        bool running = true;
        bool running_task = false;
        uint64_t cal_rate_start_time{};
        uint64_t occupied_time{};
        uint64_t last_checkpoint_time{};
        std::mutex checkpoint_lock;
        std::mutex pause_lock;
    public:
        explicit WorkerThread(ElasticExecutorImpl& impl): impl(impl) {}
        ~WorkerThread() {
            stop();
            join();
        }

        void run() override {
            using namespace std::chrono_literals;
            TaskType task;
            while (running) {
                if (pause_lock.try_lock() && impl.taskQueue.try_pop(task)) {
                    checkpoint_lock.lock();
                    last_checkpoint_time = get_time();
                    checkpoint_lock.unlock();
                    running_task = true;
                    task();
                    pause_lock.unlock();
                    running_task = false;
                    checkpoint_lock.lock();
                    occupied_time += get_time() - last_checkpoint_time;
                    checkpoint_lock.unlock();
                } else {
                    pause_lock.unlock();
                    std::this_thread::sleep_for(20ms);
                }
            }
        }

        float get_occupied_rate() {
            uint64_t now;
            std::lock_guard<std::mutex> guard(checkpoint_lock);
            if (running_task) {
                now = get_time();
                occupied_time += now - last_checkpoint_time;
                last_checkpoint_time = now;
            }
            now = get_time();
            auto dur = now - cal_rate_start_time;
            float ans = dur == 0 ? 0: float(occupied_time) / ((float)dur);
            if (now - cal_rate_start_time > occupied_rate_sample_period) {
                occupied_time = 0;
                cal_rate_start_time = now;
                last_checkpoint_time = now;
            }
            return ans;
        }

        void stop() {
            running = false;
        }

        void pause() {
            pause_lock.lock();
        }

        bool try_pause() {
            return pause_lock.try_lock();
        }

        void resume() {
            pause_lock.unlock();
        }
    };

    using TaskQueue = tbb::concurrent_queue<TaskType>;
    TaskQueue taskQueue;
    std::list<std::unique_ptr<WorkerThread>> workers;
    bool running = true;
public:
    void execute(TaskType &&func) {
        taskQueue.push(std::move(func));
    }

    void run() override {
        logd("Start Elastic Executor");
        while (running) {
           float average_rate = 0;
           std::stringstream ss;
           for (auto & worker : workers) {
               ss << worker->get_occupied_rate() << ' ';
               average_rate += worker->get_occupied_rate();
           }
           average_rate = !workers.empty() ? average_rate / (float)workers.size(): 0;
           ss << " | " <<  average_rate;
           logd("ElasticExecutor workers: {}, load {} , queue size: {}", workers.size(), ss.str(), taskQueue.unsafe_size());
           if (average_rate >= expand_threshold || workers.empty()) {
               if (taskQueue.unsafe_size() > 0) {
                   auto worker = std::make_unique<WorkerThread>(*this);
                   worker->start();
                   workers.push_back(std::move(worker));
                   logd("expand ! curren size: {}", workers.size());
               }
           }
           if (average_rate < expire_threshold) {
                for (auto it = workers.begin(); it != workers.end(); it++) {
                    if ((*it)->try_pause()) {
                        workers.erase(it);
                        logd("shrink to {}", workers.size());
                        break;
                    }
                }
           }
           std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    void stop() {
        running = false;
    }

    ~ElasticExecutorImpl() {
        logd("stop Elastic Executor");
        stop();
    }
};

NPP::ElasticExecutor::ElasticExecutor() {
    impl = new ElasticExecutorImpl();
    impl->start();
}

void NPP::ElasticExecutor::execute(TaskType &&func) {
    impl->execute(std::move(func));
}

NPP::ElasticExecutor::~ElasticExecutor() {
    delete impl;
}

void NPP::SharedElasticExecutor::execute(std::function<void()> &&func) {
    static ElasticExecutor executor;
    executor.execute(std::move(func));
}
