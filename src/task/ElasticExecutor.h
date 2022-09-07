//
// Created by WMJ on 2022/9/7.
//

#ifndef NPP_ELASTICEXECUTOR_H
#define NPP_ELASTICEXECUTOR_H

#include "Executor.h"
#include <memory>

namespace NPP {
    class ElasticExecutorImpl;
    class ElasticExecutor : public AbstractExecutor {
        ElasticExecutorImpl* impl;
    public:
        ElasticExecutor();
        ~ElasticExecutor();
        void execute(std::function<void()> &&func) override;
    };


    class SharedElasticExecutor: public AbstractExecutor {
    public:
        void execute(std::function<void()> &&func) override;
    };
}

#endif //NPP_ELASTICEXECUTOR_H
