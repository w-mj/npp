//
// Created by WMJ on 2022/9/3.
//

#include <basic/exception.h>

void f2(int index) {
    // throw NPP::RangeException("some info");
    char data[10] = {};
    for (int i = 0; i < index; i++) {
        data[i + 1] = data[i] + data[i + 1];
    }
}

void f1() {
    int b = 1 + 1;
    f2(20);
    std::cout << b << std::endl;
}

int main() {
    NPP::Exception::initExceptionHandler();
    f1();
}