//
// Created by WMJ on 2022/9/8.
//
#include "Task.h"


void NPP::TaskKeeper::check() {
    for (auto it = tasks.begin(); it != tasks.end();) {
        if (it->check()) {
            logd("TaskKeeper drop task.");
            tasks.erase(it);
            // it->del();
        } else {
            it++;
        }
    }
}
