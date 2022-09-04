//
// Created by WMJ on 2022/9/3.
//

#include "Bytes.h"
#include <cassert>
#include <cstring>

namespace NPC {
    Bytes Bytes::deepcopy() const {
        return {_data, _size};
    }

    Bytes::Bytes() {
        _size = 0;
        _data = nullptr;
    }

    Bytes::Bytes(size_t size) {
        assert(size > 0);
        this->_size = size;
        this->_data = new unsigned char[size];
    }

    Bytes::Bytes(void* data, size_t size) {
        assert(size > 0);
        assert(data != nullptr);
        this->_size = size;
        this->_data = new unsigned char[size];
        memcpy(this->_data, data, size);
    }

    Bytes::Bytes(Bytes &&ano) noexcept{
        _size = ano._size;
        _data = ano._data;
        ano._data = nullptr;
    }

    Bytes::~Bytes() {
        delete[] _data;
    }

    size_t Bytes::size() const {
        return _size;
    }

    void Bytes::set(size_t size, void *data, size_t start) {
        assert(start + size <= this->size());
        memcpy(this->_data + start, data, size);
    }

} // NPC