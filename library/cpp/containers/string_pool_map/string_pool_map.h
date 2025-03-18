#pragma once

#include <library/cpp/containers/str_map/str_map.h>
#include <util/str_stl.h>
#include <util/generic/hash.h>
#include <util/generic/map.h>
#include <util/generic/strbuf.h>
#include <util/memory/segmented_string_pool.h>
#include <util/generic/noncopyable.h>

template <class T>
class TStringBufAbstractMapWithKeysPool: protected  T, TNonCopyable {
private:
    TStringBufAbstractMapWithKeysPool() = default;

protected:
    segmented_pool<char> Pool;

public:
    using TBase = T;
    using iterator = typename TBase::iterator;
    using const_iterator = typename TBase::const_iterator;
    using mapped_type = typename TBase::mapped_type;
    using size_type = typename TBase::size_type;
    using key_type = typename TBase::key_type;
    using value_type = typename TBase::value_type;

    TStringBufAbstractMapWithKeysPool(size_type hash_size = HASH_SIZE_DEFAULT, size_t segsize = HASH_SIZE_DEFAULT * AVERAGEWORD_BUF, bool afs = false)
        : TBase(hash_size)
        , Pool(segsize)
    {
        Y_UNUSED(hash_size);
        if (afs)
            Pool.alloc_first_seg();
    }

    TStringBufAbstractMapWithKeysPool(size_t segsize = HASH_SIZE_DEFAULT * AVERAGEWORD_BUF, bool afs = false)
        : TBase()
        , Pool(segsize)
    {
        if (afs)
            Pool.alloc_first_seg();
    }

    std::pair<iterator, bool> insert(TStringBuf key, const mapped_type& data) {
        std::pair<iterator, bool> ins = TBase::insert(value_type(key, data));
        if (ins.second && !key.empty()) {
            const char* copyPtr = Pool.append(key.begin(), key.length());
            (TStringBuf&)(*ins.first).first = TStringBuf(copyPtr, copyPtr + key.length());
        }
        return ins;
    }

    mapped_type& operator[](TStringBuf key) {
        iterator I = TBase::find(key);
        if (I == TBase::end())
            I = insert(key, mapped_type()).first;
        return (*I).second;
    }

    void clear() {
        TBase::clear();
        Pool.restart();
    }

    size_t pool_size() const {
        return Pool.size();
    }

    using TBase::begin;
    using TBase::empty;
    using TBase::end;
    using TBase::find;
    using TBase::contains;
    using TBase::size;
    using TBase::at;
};

template <class T>
class TStringBufHashWithKeysPool: public TStringBufAbstractMapWithKeysPool<THashMap<TStringBuf, T, THash<TStringBuf>>> {
public:
    using TBase = TStringBufAbstractMapWithKeysPool<THashMap<TStringBuf, T, THash<TStringBuf>>>;
    using iterator = typename TBase::iterator;
    using const_iterator = typename TBase::const_iterator;
    using mapped_type = typename TBase::mapped_type;
    using size_type = typename TBase::size_type;
    using key_type = typename TBase::key_type;
    using value_type = typename TBase::value_type;

public:
    TStringBufHashWithKeysPool(size_type hash_size = HASH_SIZE_DEFAULT, size_t segsize = HASH_SIZE_DEFAULT * AVERAGEWORD_BUF, bool afs = false)
        : TBase(hash_size, segsize, afs)
    {
    }
};

template <class T>
class TStringBufMapWithKeysPool: public TStringBufAbstractMapWithKeysPool<TMap<TStringBuf, T, TLess<TStringBuf>>> {
public:
    using TBase = TStringBufAbstractMapWithKeysPool<TMap<TStringBuf, T, TLess<TStringBuf>>>;
    using iterator = typename TBase::iterator;
    using const_iterator = typename TBase::const_iterator;
    using mapped_type = typename TBase::mapped_type;
    using size_type = typename TBase::size_type;
    using key_type = typename TBase::key_type;
    using value_type = typename TBase::value_type;

public:
    TStringBufMapWithKeysPool(size_t segsize = HASH_SIZE_DEFAULT * AVERAGEWORD_BUF, bool afs = false)
        : TBase(segsize, afs)
    {
    }
};
