//
// Created by WMJ on 2022/9/8.
//

#include <iostream>
#include <type_traits>

using namespace std;

template<typename T>
struct Gen {
    T gen() {
        T a = T{};
        a++;
        return a;
    }
};

template<>
struct Gen<void> {
    void gen() {};
};

template<typename T>
struct TestClass1 {
    T get_result() {
        Gen<T> g;
        return g.gen();
    }

    std::string name() requires (!is_void_v<T>){
        T a{};
        a++;
        return std::string("not void ") + to_string(get_result() + a);
    }

    std::string name() requires (is_void_v<T>) {
        return "void";
    }
};

int main() {
    cout << TestClass1<int>().name() << endl;
    cout << TestClass1<void>().name() << endl;
}