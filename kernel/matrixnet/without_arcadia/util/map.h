#pragma once

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
#error This header file is intended for use in the MATRIXNET_WITHOUT_ARCADIA mode only.
#endif

#include <map>

namespace NMatrixnet {

template<typename K, typename V>
class TMap : public std::map<K, V> {
public:
    template<typename X>
    const V* FindPtr(const X& key) const {
        auto it = this->find(key);
        if (it == this->end())
            return nullptr;
        return &it->second;
    }

    template<typename X>
    V* FindPtr(const X& key) {
        auto it = this->find(key);
        if (it == this->end())
            return nullptr;
        return &it->second;
    }

    void Swap(TMap<K, V>& other) {
        this->swap(other);
    }
    template<typename X>
    bool has(const X& key) const {
        return this->find(key) != this->end();
    }
};

} // namespace NMatrixnet
