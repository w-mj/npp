//
// Created by WMJ on 2022/9/3.
//

#ifndef NPP_NETWORKMANAGER_H
#define NPP_NETWORKMANAGER_H

#include <string>
#include <unordered_map>
#include <basic/Bytes.h>
#include <basic/Thread.h>
#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_queue.h>

namespace NPP {

    struct PacketHead {
        uint16_t control;
        uint16_t reserved;
        uint32_t size;
    } __attribute((packed));

    class NetworkManager : Thread {
        int myRank;
        bool running;
        int sockfd;
        int epoll_fd;
        uint16_t listenPort;  // 服务器监听地址，使用本机字节序
        void run() override;
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
        Bytes getMessage();
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
        void processMessage(PacketHead head, Bytes content, uint32_t verify);

        using MessageQueue = tbb::concurrent_bounded_queue<Bytes>;
        MessageQueue messageQueue;
    };

} // NPP

#endif //NPP_NETWORKMANAGER_H
