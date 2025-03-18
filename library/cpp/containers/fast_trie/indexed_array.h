#pragma once

#include <util/system/defaults.h>
#include <utility>

// A static array with THashMap interface
// Must be provided with TraitsType, defining key type (TraitsType::TKey) and array size (TraitsType::Size)
// and implementing projection from TraitsType::TKey to [0, TraitsType::Size) in Z (TraitsType::Index(const TraitsType::TKey&))
// with key initialization check (bool TraitsType::Initialized(const TraitsType::TKey&)) which distinguishes default key value from non-default

struct TCharArrayTraits {
    typedef char TKey;

    enum {
        // Number of letters in alphabet
        Size = 256,
    };

    static bool Initialized(char key) {
        return key != 0;
    }

    // A function which maps a character to a number in range 0 .. Size - 1.
    static size_t Index(char key) {
        return static_cast<unsigned char>(key);
    }
};

template <typename ValueType, typename TraitsType>
class TIndexedArray {
public:
    typedef ValueType mapped_type;
    typedef TraitsType TTraits;
    typedef typename TTraits::TKey key_type;
    typedef std::pair<key_type, mapped_type> value_type;
    typedef value_type* iterator;
    typedef const value_type* const_iterator;

public:
    TIndexedArray() = default;

    iterator begin() {
        return Data;
    }

    const_iterator begin() const {
        return Data;
    }

    iterator end() {
        return Data + TTraits::Size;
    }

    const_iterator end() const {
        return Data + TTraits::Size;
    }

    // Partial matching (equal index, different value) is allowed
    iterator find(const key_type& key) {
        if (!TTraits::Initialized(key))
            return end();
        size_t index = TTraits::Index(key);
        return index < TTraits::Size && TTraits::Initialized(Data[index].first) ? &Data[index] : end();
    }

    const_iterator find(const key_type& key) const {
        if (!TTraits::Initialized(key))
            return end();
        size_t index = TTraits::Index(key);
        return index < TTraits::Size && TTraits::Initialized(Data[index].first) ? &Data[index] : end();
    }

    std::pair<iterator, bool> insert(const value_type& val) {
        if (TTraits::Initialized(val.first)) {
            size_t index = TTraits::Index(val.first);
            if (index < TTraits::Size) {
                Data[index] = val;
                return std::make_pair(&Data[index], false);
            }
        }
        return std::make_pair(iterator(nullptr), false);
    }

    void erase(const key_type&) {
        return;
    }

private:
    value_type Data[TTraits::Size];
};
