//
// Created by WMJ on 2022/9/3.
//

#include "Bytes.h"
#include <cassert>
#include <cstring>
#include "basic/Exception.h"

namespace NPP {
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

    Bytes::Bytes(const void* data, size_t size) {
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
        ano._size = 0;
    }

    Bytes::~Bytes() {
        delete[] _data;
    }

    size_t Bytes::size() const {
        return _size;
    }

    void Bytes::set(size_t size, void *data, size_t start) {
        if (start + size > this->size()) {
            throw RangeException(fmt::format("{} + {} > {}", start, size, this->size()).c_str());
        }
        memcpy(this->_data + start, data, size);
    }

    Bytes& Bytes::operator=(Bytes &&ano)  noexcept {
        _size = ano._size;
        _data = ano._data;
        ano._data = nullptr;
        ano._size = 0;
        return *this;
    }

} // NPP