#include <iostream>

int main() {
    if constexpr(true) {
        std::cout << "Hello, World!" << std::endl;
    } else {
        aa;
        std::cout << "Bye, World!" << std::endl;
    }
    return 0;
}
