#include <iostream>

int main() {
    if constexpr(true) {
        std::cout << "Hello, World!" << std::endl;
    } else {
        std::cout << "Bye, World!" << std::endl;
    }
    return 0;
}
