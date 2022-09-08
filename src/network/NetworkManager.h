//
// Created by WMJ on 2022/9/3.
//

#ifndef NPP_NETWORKMANAGER_H
#define NPP_NETWORKMANAGER_H

#include <string>
#include <unordered_map>
#include <basic/Bytes.h>
#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_queue.h>
#include <task/Task.h>
#include <task/Channel.h>

namespace NPP {

    struct MessageHead {
        uint16_t compress :1;
        uint16_t verify :1;
        uint16_t res1 :6;
        uint16_t type: 3;
        uint16_t res2: 5;

        uint16_t reserved;
        uint32_t size;

        std::string repr() const {
            int t = type;
            return fmt::format("MessageHead[type:{}, size:{}]", t, (int)size);
        }
    } __attribute((packed));

    class NetworkManager {
        int myRank;
        bool running;
        int sockfd;
        int epoll_fd;
        uint16_t listenPort;  // 服务器监听地址，使用本机字节序
        Task<void, NewThreadExecutor> run();
        Task<void, NewThreadExecutor> networkTask;
    public:
        explicit NetworkManager(int myRank);
        ~NetworkManager();

        // 使用本机字节序
        void startServer(uint16_t port=0);
        void stopServer();
        [[nodiscard]] uint16_t getMyPort() const;

        // 注册目标rank和地址关系，使用本机字节序
        void registerTarget(int rank, uint32_t ip, uint16_t port);
        void registerTarget(int rank, std::string_view ip, uint16_t port);
        // 发送数据
        bool sendMessage(int rank, Bytes data, int retry=0);
        // 阻塞，直到收到消息
        Task<std::shared_ptr<Bytes>> getMessage();
    private:
        struct Target {
            int rank = -1;
            uint32_t ip = 0;
            uint16_t port = 0;
            int sock = -1;
        };
        using SockMap = tbb::concurrent_hash_map<int, Target>;
        SockMap socks;
        int getSendSocket(int rank);
        void clearSendSocket(int rank);
        Task<void> processMessage(MessageHead head, Bytes content, uint32_t verify);
        void closeSocket(int sock) const;

        void readNormalMessage(int connfd, const MessageHead& message_head);
        void readRankReportMessage(int connfd, const MessageHead& message_head);
        bool sendRankReportMessage(int connfd) const;
        uint32_t readVerify(int connfd);

//        using MessageQueue = tbb::concurrent_bounded_queue<Bytes>;
//        MessageQueue messageQueue;
        Channel<std::shared_ptr<Bytes>> messageChannel;
    };

} // NPP

#endif //NPP_NETWORKMANAGER_H
