//
// Created by WMJ on 2022/9/3.
//

#include "NetworkManager.h"
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/uio.h>
#include <netdb.h>
#include <arpa/inet.h>


namespace NPP {


    void NetworkManager::startServer(uint16_t port) {
        this->listenPort = port;
        start();
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
        value->second.sock = sock;
        return sock;
    }

    void NetworkManager::run() {
        constexpr int epoll_event_size = 256;
        constexpr int receive_buff_size = 4096;
        char receive_buff[receive_buff_size];
        PacketHead message_head{};
        uint32_t verify_code;

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd == -1){
            printf("socket error!\n");
            return;
        }

        struct sockaddr_in serv_addr{};
        struct sockaddr_in clit_addr{};
        socklen_t clit_len;


        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(listenPort);
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 任意本地ip

        int ret = bind(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
        if(ret == -1){
            printf("bind error!\n");
            return;
        }

        listen(sockfd, 10);

        //创建epoll
        epoll_fd = epoll_create(epoll_event_size);
        if(epoll_fd == -1){
            printf("epoll_create error!\n");
            return ;
        }

        //向epoll注册sockfd监听事件
        struct epoll_event ev{};   //epoll事件结构体
        struct epoll_event events[epoll_event_size];  //事件监听队列

        ev.events = EPOLLIN;
        ev.data.fd = sockfd;

        int ret2 = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &ev);
        if(ret2 == -1){
            printf("epoll_ctl error!\n");
            return ;
        }
        socklen_t socklen = sizeof(serv_addr);
        getsockname(sockfd, (struct sockaddr*)&serv_addr, &socklen);
        listenPort = ntohs(serv_addr.sin_port);

        // inited
        running = true;
        while(running){
            int nfds = epoll_wait(epoll_fd,events,epoll_event_size,-1);
            if(nfds == -1){
                printf("epoll_wait error!\n");
                return ;
            }
            for(int i = 0; i < nfds; ++i){
                if(events[i].data.fd == sockfd){
                    // 接受新连接
                    int connfd =  accept(sockfd, (struct sockaddr*)&clit_addr, &clit_len);
                    if(connfd == -1){
                        printf("accept error!\n");
                        return ;
                    }

                    ev.events = EPOLLIN;
                    ev.data.fd = connfd;
                    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connfd, &ev) == -1){
                        printf("epoll_ctl add error!\n");
                        return;
                    }
                }
                else{
                    // 读取数据
                    int connfd = events[i].data.fd;
                    ssize_t ret1 = read(connfd, &message_head, sizeof(message_head));
                    if (ret1 <= 0) {
                        if (ret1 < 0) {
                            printf("receive head error ret=%zd errno=%d.\n", ret1, errno);
                        }
                        close(connfd);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, connfd, nullptr);
                        continue;
                    }
                    Bytes data(message_head.size);
                    uint32_t remain_size = message_head.size;
                    uint32_t all_read_size = 0;
                    while (remain_size > 0) {
                        auto this_read_size = std::min((uint32_t)receive_buff_size, remain_size);
                        ssize_t read_size = read(connfd, receive_buff, this_read_size);
                        if (read_size <= 0) {
                            if (read_size < 0) {
                                printf("receive error ret=%zd errno=%d.\n", read_size, errno);
                            }
                            close(connfd);
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, connfd, nullptr);
                            break;
                        }
                        data.set(read_size, receive_buff, all_read_size);
                        all_read_size += read_size;
                        remain_size -= read_size;
                    }
                    if (remain_size > 0) {
                        continue;
                    }
                    ret1 = read(connfd, &verify_code, sizeof(verify_code));
                    if (ret1 <= 0) {
                        if (ret1 < 0) {
                            printf("receive verify code error ret=%zd errno=%d.\n", ret1, errno);
                        }
                        close(connfd);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, connfd, nullptr);
                    }
                    // 读取完成
                    processMessage(message_head, std::move(data), verify_code);
                }
            }
        }
        printf("server stop!\n");
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

    Bytes NetworkManager::getMessage() {
        Bytes result;
        messageQueue.pop(result);
        return result;
    }

    bool NetworkManager::sendMessage(int rank, Bytes data, int retry) {
        int sock = getSendSocket(rank);
        if (sock < 0) {
            return retry > 0 && sendMessage(rank, std::move(data), retry - 1);
        }
        PacketHead head{.control = 0, .size = static_cast<uint32_t>(data.size())};
        uint32_t verify = 0xDEADFACE;
        iovec vec[3] = {
                {.iov_base = &head, .iov_len = sizeof(PacketHead)},
                {.iov_base = &data.as<char>(), .iov_len = data.size()},
                {.iov_base = &verify, .iov_len = sizeof(verify)}
        };
        size_t all_size = vec[0].iov_len + vec[1].iov_len + vec[2].iov_len;
        ssize_t ret = writev(sock, vec, 3);
        if (ret < 0) {
            clearSendSocket(rank);
            return retry > 0 && sendMessage(rank, std::move(data), retry - 1);
        } else if (ret < all_size) {
            printf("!!!WARNING!!! sendMessage %zd < %zd", ret, all_size);
            return false;
        }
        return true;
    }

    void NetworkManager::registerTarget(int rank, uint32_t ip, uint16_t port) {
        printf("register Target %d %s:%d\n", rank, inet_ntoa({.s_addr = htonl(ip)}), port);
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

    void NetworkManager::processMessage(PacketHead head, Bytes content, uint32_t verify) {
        messageQueue.push(std::move(content));
    }

    void NetworkManager::stopServer() {
        running = false;
        if (sockfd > 0) {
            close(sockfd);
        }
        if (epoll_fd > 0) {
            close(epoll_fd);
        }
        listenPort = 0;
        join();
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


} // NPP