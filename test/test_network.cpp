//
// Created by WMJ on 2022/9/6.
//

#include <network/NetworkManager.h>


int main(int argc, char **argv) {
    int myRank = 0;
    if (argc == 2) {
        myRank = 1;
    }
    NPP::NetworkManager network(myRank);
    if (myRank == 0) {
        // server
        network.startServer();
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
        auto data = network.getMessage();
        printf("receive : %s\n", &data.as<char>());
    } else {
        network.registerTarget(0, "127.0.0.1", atoi(argv[1]));
        const char *s = "Hello Network!";
        NPP::Bytes data(s, strlen(s) + 1);
        network.sendMessage(0, std::move(data));
    }
}