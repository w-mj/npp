//
// Created by WMJ on 2022/9/3.
//

#ifndef NPC_BYTES_H
#define NPC_BYTES_H

#include <cstddef>
#include <memory>

namespace NPC {
    class Bytes {
        size_t _size;
        unsigned char* _data;
    public:
        Bytes();
        explicit Bytes(size_t size);
        Bytes(void *data, size_t size);
        // 关闭复制构造函数，复制构造应使用deepcopy()函数，防止不必要的复制构造带来性能开销
        Bytes(const Bytes&) = delete;
        Bytes& operator=(const Bytes&) = delete;
        // 移动构造
        Bytes(Bytes&& ano) noexcept;
        ~Bytes();

        [[nodiscard]] Bytes deepcopy() const;

        template<typename T> T& as() {
            return *static_cast<T*>(_data);
        }

        [[nodiscard]] size_t size() const;

        void set(size_t size, void *data, size_t start=0);
    };

} // NPC

#endif //NPC_BYTES_H
