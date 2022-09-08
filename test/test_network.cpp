//
// Created by WMJ on 2022/9/6.
//

#include <network/NetworkManager.h>


Task<void> testNetwork(int argc, char **argv) {
    int myRank = 0;
    if (argc == 2) {
        myRank = 1;
    }
    NPP::NetworkManager network(myRank);
    network.startServer();
    if (myRank == 0) {
        // server
        while (network.getMyPort() == 0) {
            ;
        }
        printf("network thread start with port %d\n", network.getMyPort());
        if (fork() == 0) {
            char buf[20];
            sprintf(buf, "%d", network.getMyPort());
            char *const aa[] = {
                    argv[0], buf, nullptr
            };
            execv(argv[0], aa);
        }
        auto data = co_await network.getMessage();
        printf("0: receive : %s\n", &(data->as<char>()));
        network.sendMessage(1, {"aaa", 4});
        sleep(2);
    } else {
        network.registerTarget(0, "127.0.0.1", atoi(argv[1]));
        const char *s = "Hello Network!";
        NPP::Bytes data(s, strlen(s) + 1);
        network.sendMessage(0, std::move(data));

        auto res = co_await network.getMessage();
        printf("1: receive : %s\n", &res->as<char>());
    }
}

int main(int argc, char **argv) {
    testNetwork(argc, argv).get_result();
    return 0;
}