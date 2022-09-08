//
// Created by WMJ on 2022/9/3.
//

#include "NetworkManager.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/uio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <cassert>
#include <basic/Logger.h>


static constexpr uint32_t fix_verify_code = 0xDEADFACE;   // 3735943886


namespace NPP {

    void NetworkManager::startServer(uint16_t port) {
        this->listenPort = port;
        networkTask = std::move(run());
        // start();
    }

    int NetworkManager::getSendSocket(int rank) {
        SockMap::accessor value;
        if (!socks.find(value, rank)) {
            // not register;
            return -1;
        }
        if (value->second.sock > 0) {
            return value->second.sock;
        }

        int sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in server{};

        if (sock == -1) {
            return -1;
        }

        server.sin_addr.s_addr = value->second.ip;
        server.sin_family = AF_INET;
        server.sin_port = value->second.port;

        if (connect(sock, (struct sockaddr *)&server , sizeof(server)) < 0) {
            return -1;
        }
        running.wait(false);
        if (!sendRankReportMessage(sock)) {
            close(sock);
            return -1;
        }
        value->second.sock = sock;

        struct epoll_event ev{};   //epoll事件结构体
        ev.events = EPOLLIN;
        ev.data.fd = sock;
        int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &ev);
        if (ret < 0) {
            loge("epoll_ctl error ret: {}, errno: {}", ret, errno);
        }
        return sock;
    }

    Task<void> NetworkManager::run() {
        constexpr int epoll_event_size = 256;

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd == -1){
            loge("socket error!");
            co_return;
        }

        struct sockaddr_in serv_addr{};
        struct sockaddr_in clit_addr{};
        socklen_t clit_len = sizeof(clit_addr);


        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(listenPort);
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 任意本地ip

        int ret = bind(sockfd, (struct sockaddr*)&serv_addr,sizeof(serv_addr));
        if(ret == -1){
            loge("bind error!");
            co_return;
        }

        listen(sockfd, 10);

        //创建epoll
        epoll_fd = epoll_create(epoll_event_size);
        if(epoll_fd == -1){
            loge("epoll_create error!");
            co_return;
        }

        //向epoll注册sockfd监听事件
        struct epoll_event ev{};   //epoll事件结构体
        struct epoll_event events[epoll_event_size];  //事件监听队列

        ev.events = EPOLLIN ;
        ev.data.fd = sockfd;

        int ret2 = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &ev);
        if(ret2 == -1){
            loge("epoll_ctl error!");
            co_return;
        }
        socklen_t socklen = sizeof(serv_addr);
        getsockname(sockfd, (struct sockaddr*)&serv_addr, &socklen);
        listenPort = ntohs(serv_addr.sin_port);
        logd("start listen at {}", listenPort);
        // inited
        running.store(true);
        running.notify_all();
        while(running){
            int nfds = epoll_wait(epoll_fd, events, epoll_event_size, 120);
            if(nfds == -1){
                loge("epoll_wait error!");
                co_return;
            }
            for(int i = 0; i < nfds; ++i){
                if(events[i].data.fd == sockfd){
                    // 接受新连接
                    int connfd = accept(sockfd, (struct sockaddr*)&clit_addr, &clit_len);
                    if(connfd == -1){
                        loge("accept error! errno: {}", strerror(errno));
                        running = false;
                        continue ;
                    }

                    ev.events = EPOLLIN;
                    ev.data.fd = connfd;
                    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connfd, &ev) == -1){
                        loge("epoll_ctl add error!");
                        continue ;
                    }
                    logd("accept target fd {}", connfd);
                }
                else{
                    // 读取数据
                    MessageHead message_head{};
                    int connfd = events[i].data.fd;
                    ssize_t ret1 = read(connfd, &message_head, sizeof(message_head));
                    if (ret1 <= 0) {
                        if (ret1 < 0) {
                            loge("receive head error ret=%zd errno=%d.", ret1, errno);
                        }
                        closeSocket(connfd);
                        continue;
                    }
                    logd(message_head.repr());
                    switch (message_head.type) {
                        case 0: {
                            readNormalMessage(connfd, message_head);
                            break;
                        }
                        case 1: {
                            readRankReportMessage(connfd, message_head);
                            break;
                        }
                        default: {
                            loge("Unknown message type %d.", message_head.type);
                        }
                    }
                }
            }
        }
        logd("server stop! rank:{}", myRank);
        close(epoll_fd);
        close(sockfd);
    }

    NetworkManager::NetworkManager(int myRank) {
        this->myRank = myRank;
        this->epoll_fd = -1;
        this->running = false;
        this->sockfd = -1;
        this->listenPort = 0;
    }

    Task<std::shared_ptr<Bytes>> NetworkManager::getMessage() {
        std::shared_ptr<Bytes> result;
        result = co_await messageChannel.read();
        // messageQueue.pop(result);
        co_return result;
    }

    bool NetworkManager::sendMessage(int rank, Bytes data, int retry) {
        int sock = getSendSocket(rank);
        if (sock < 0) {
            return retry > 0 && sendMessage(rank, std::move(data), retry - 1);
        }
        MessageHead head{};
        head.size = static_cast<uint32_t>(data.size());
        uint32_t verify = fix_verify_code;
        iovec vec[3] = {
                {.iov_base = &head, .iov_len = sizeof(MessageHead)},
                {.iov_base = &data.as<char>(), .iov_len = data.size()},
                {.iov_base = &verify, .iov_len = sizeof(verify)}
        };
        size_t all_size = vec[0].iov_len + vec[1].iov_len + vec[2].iov_len;
        ssize_t ret = writev(sock, vec, 3);
        logd("send message rank: {}, fd: {}, size: {}, ret: {}", rank, sock, all_size, ret);
        if (ret < 0) {
            clearSendSocket(rank);
            return retry > 0 && sendMessage(rank, std::move(data), retry - 1);
        } else if (ret < all_size) {
            loge("!!!WARNING!!! sendMessage {} < {}", ret, all_size);
            return false;
        }
        return true;
    }

    void NetworkManager::registerTarget(int rank, uint32_t ip, uint16_t port) {
        logd("register Target {} {}:{}", rank, inet_ntoa({.s_addr = htonl(ip)}), port);
        SockMap::accessor accessor;
        if (socks.find(accessor, rank)) {
            accessor->second.ip = htonl(ip);
            accessor->second.port = htons(port);
            accessor->second.sock = -1;
        } else {
            Target t{.rank = rank, .ip=htonl(ip), .port=htons(port)};
            SockMap::value_type newValue(rank, t);
            socks.insert(newValue);
        }
    }


    void NetworkManager::registerTarget(int rank, std::string_view ip, uint16_t port) {
        struct hostent *he;
        struct in_addr **addr_list;
        int i;

        if ((he = gethostbyname( ip.data() ) ) == nullptr) {
            in_addr addr{};
            inet_aton(ip.data(), &addr);
            registerTarget(rank, ntohl(addr.s_addr), port);
        } else {
            addr_list = (struct in_addr **) he->h_addr_list;
            for (i = 0; addr_list[i] != nullptr; i++) {
                registerTarget(rank, ntohl(addr_list[i]->s_addr), port);
            }
        }
    }

    uint16_t NetworkManager::getMyPort() const {
        return listenPort;
    }

    void NetworkManager::processMessage(MessageHead head, Bytes content, uint32_t verify) {
        // messageQueue.push(std::move(content));
        messageChannel.write_sync(std::make_shared<Bytes>(std::move(content)));
    }

    void NetworkManager::stopServer() {
        running = false;
        listenPort = 0;
        if (sockfd > 0) {
            close(sockfd);
            sockfd = -1;
        }
        if (epoll_fd > 0) {
            close(epoll_fd);
            epoll_fd = -1;
        }
        networkTask.get_result();
        // join();
    }

    void NetworkManager::clearSendSocket(int rank) {
        SockMap::accessor accessor;
        if (!socks.find(accessor, rank)) {
            return;
        }
        accessor->second.sock = -1;
    }

    NetworkManager::~NetworkManager() {
        if (running) {
            stopServer();
        }
    }

    void NetworkManager::closeSocket(int connfd) const {
        // logd("socket close {}", connfd);
        close(connfd);
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, connfd, nullptr);
    }

    void NetworkManager::readNormalMessage(int connfd, const MessageHead& message_head) {
        // read content
        constexpr int receive_buff_size = 4096;
        char receive_buff[receive_buff_size];
        uint32_t verify_code;
        Bytes data(message_head.size);
        uint32_t remain_size = message_head.size;
        uint32_t all_read_size = 0;
        while (remain_size > 0) {
            auto this_read_size = std::min((uint32_t) receive_buff_size, remain_size);
            ssize_t read_size = read(connfd, receive_buff, this_read_size);
            if (read_size <= 0) {
                closeSocket(connfd);
                break;
            }
            data.set(read_size, receive_buff, all_read_size);
            all_read_size += read_size;
            remain_size -= read_size;
        }
        if (remain_size > 0) {
            return;
        }

        verify_code = readVerify(connfd);
        assert(verify_code == fix_verify_code);
        logd("receive message size: {}, verify: {:#x}", all_read_size, verify_code);
        processMessage(message_head, std::move(data), verify_code);
    }

    void NetworkManager::readRankReportMessage(int connfd, const MessageHead& message_head) {
        uint32_t verify_code = readVerify(connfd);
        assert(verify_code == fix_verify_code);
        struct sockaddr_in addr{};
        socklen_t socklen = sizeof(addr);
        getsockname(connfd, (struct sockaddr*)&addr, &socklen);
        uint32_t  ip = addr.sin_addr.s_addr;  // 网络字节序
        uint16_t port = message_head.reserved;  // 网络字节序
        int rank = (int)message_head.size;
        Target t{.rank = rank, .ip = ip, .port = port, .sock = connfd};
        SockMap::value_type value(rank, t);
        socks.insert(value);
        logd("receive rank: {}, ip: {}, port: {}, sock: {}", rank, ip, port, connfd);
    }

    uint32_t NetworkManager::readVerify(int connfd) {
        uint32_t verify_code;
        // read verify
        ssize_t ret1 = read(connfd, &verify_code, sizeof(verify_code));
        if (ret1 <= 0) {
            closeSocket(connfd);
        }
        return verify_code;
    }

    bool NetworkManager::sendRankReportMessage(int connfd) const {
        MessageHead head{};
        head.type = 1;
        head.size = myRank;
        head.reserved = htons(listenPort);
        uint32_t verify = fix_verify_code;
        iovec vec[2] = {
                {.iov_base = &head, .iov_len = sizeof(MessageHead)},
                {.iov_base = &verify, .iov_len = sizeof(verify)}
        };
        size_t all_size = vec[0].iov_len + vec[1].iov_len;
        ssize_t ret = writev(connfd, vec, 2);
        if (ret < 0) {
            return false;
        } else if (ret < all_size) {
            return false;
        }
        return true;
    }


} // NPP