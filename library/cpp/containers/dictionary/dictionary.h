#pragma once

#include <library/cpp/digest/lower_case/hash_ops.h>
#include <util/draft/enum.h>
#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/memory/pool.h>
#include <util/string/cast.h>

namespace NNameIdDictionary {
    // for tightly ordered data
    template <class IdT, class StrT = TStringBuf>
    class TSolidId2Str : TVector<StrT> {
        using TParent = TVector<StrT>;

    public:
        using TParent::size;
        void Insert(const StrT& str, IdT id) {
            if (size_t(id) != size())
                ythrow yexception() << "Bad ordered data on id: " << size_t(id) << ", string: " << str;
            TParent::push_back(str);
        }

        const StrT* Find(IdT id) const {
            if (size_t(id) >= size())
                return nullptr;
            return &(*this)[id];
        }
    };

    template <class TDict>
    static const typename TDict::mapped_type* DictFind(const TDict& dict, const typename TDict::key_type& key) {
        typename TDict::const_iterator i = dict.find(key);
        if (i == dict.end())
            return nullptr;
        return &i->second;
    }

    template <class IdT, class StrT = TStringBuf>
    class TSparseId2Str : THashMap<IdT, StrT> {
        using TParent = THashMap<IdT, StrT>;

    public:
        using TParent::size;
        void Insert(const StrT& str, IdT id) {
            (*this)[id] = str;
        }

        const StrT* Find(IdT id) const {
            return DictFind<TParent>(*this, id);
        }
    };

    template <class IdT>
    class TStr2Id: public THashMap<TStringBuf, IdT> {
        using TParent = THashMap<TStringBuf, IdT>;

    public:
        using TParent::size;
        void Insert(const TStringBuf& str, IdT id) {
            (*this)[str] = id;
        }

        const IdT* Find(const TStringBuf& str) const {
            return DictFind<TParent>(*this, str);
        }
    };

    template <class IdT>
    class TCaseInsensitiveStr2Id: public THashMap<TStringBuf, IdT, TCIOps, TCIOps> {
        using TParent = THashMap<TStringBuf, IdT, TCIOps, TCIOps>;

    public:
        using TParent::size;

        void Insert(const TStringBuf& str, IdT id) {
            (*this)[str] = id;
        }

        const IdT* Find(const TStringBuf& str) const {
            return DictFind<TParent>(*this, str);
        }
    };

    // string storage policy
    template <typename StrT>
    class TStringStorage {
    };

    // TString stores itself
    template <>
    class TStringStorage<TString> {
    public:
        TString Store(const TString& s) {
            return s;
        }
    };

    // TStringBufs are stored in string pool
    template <>
    class TStringStorage<TStringBuf> {
    public:
        TStringStorage()
            : Pool(1024)
        {
        }

        TStringBuf Store(const TStringBuf& str) {
            return Pool.AppendCString(str); // also store terminating zero
        }

    private:
        TMemoryPool Pool;
    };

}

template <class IdT, class StrT = TStringBuf,
          class TId2String = NNameIdDictionary::TSolidId2Str<IdT, StrT>,
          class TString2Id = NNameIdDictionary::TStr2Id<IdT>>
class TEnum2String: public TNonCopyable {
private:
    NNameIdDictionary::TStringStorage<StrT> Storage;

    TId2String Id2String;
    TString2Id String2Id;

public:
    TEnum2String(size_t poolSegSize = 0) {
        Y_UNUSED(poolSegSize);
    }

    template <size_t N>
    TEnum2String(const std::pair<const char*, IdT> (&data)[N]) {
        Init(data, N);
    }

    // constructor for enum mapping (id <-> name)
    TEnum2String(const std::pair<const char*, IdT>* data, size_t size) {
        Init(data, size);
    }

    template <size_t N>
    void Init(const std::pair<const char*, IdT> (&data)[N]) {
        Init(data, N);
    }

    void Init(const std::pair<const char*, IdT>* data, size_t size) {
        if (Size() != 0)
            ythrow yexception() << "Cannot reinit non-empty dictionary";

        for (size_t i = 0; i < size; ++i)
            Insert(data[i].first, data[i].second);
    }

    size_t Size() const {
        return Id2String.size();
    }

    IdT Name2Id(const TStringBuf& str) const {
        const IdT* id = FindName(str);
        if (!id)
            ythrow yexception() << "There's no id with such string in Dictionary: " << str;
        return *id;
    }

    bool TryName2Id(const TStringBuf& str, IdT& result) const {
        const IdT* id = FindName(str);
        if (id) {
            result = *id;
            return true;
        }
        return false;
    }

    const StrT& Id2Name(IdT id) const {
        const StrT* str = FindId(id);
        if (!str)
            ythrow yexception() << "There's no string with such id in Dictionary: " << size_t(id);
        return *str;
    }

    bool TryId2Name(IdT id, StrT& result) const {
        const StrT* str = FindId(id);
        if (str) {
            result = *str;
            return true;
        }
        return false;
    }

    const IdT* FindName(const TStringBuf& str) const {
        return String2Id.Find(str);
    }

    const StrT* FindId(IdT id) const {
        return Id2String.Find(id);
    }

    void GetAllNames(TVector<std::pair<IdT, StrT>>& names) const {
        for (typename TString2Id::const_iterator it = String2Id.begin(); it != String2Id.end(); ++it) {
            names.push_back(std::make_pair(it->second, it->first));
        }

        Sort(names.begin(), names.end());
    }

    void Insert(const StrT& str, IdT id) {
        if (FindName(str))
            ythrow yexception() << "Already has string in Dictionary: " << str;
        if (FindId(id))
            ythrow yexception() << "Already has id in Dictionary: " << size_t(id);

        StrT stored = Storage.Store(str);

        Id2String.Insert(stored, id);
        String2Id.Insert(stored, id);
    }
};

template <class IdT, class StrT, class TId2String, class TString2Id = NNameIdDictionary::TStr2Id<IdT>> //IdT must be integer type
class TStringIdDictionaryOperators: public TEnum2String<IdT, StrT, TId2String, TString2Id> {
    using TParent = TEnum2String<IdT, StrT, TId2String, TString2Id>;

public:
    TStringIdDictionaryOperators(size_t poolSegSize = 0)
        : TParent(poolSegSize)
    {
    }

    // constructor for enum mapping (id <-> name)
    TStringIdDictionaryOperators(const std::pair<const char*, IdT>* data, size_t size)
        : TParent(data, size)
    {
    }

    template <size_t N>
    TStringIdDictionaryOperators(const std::pair<const char*, IdT> (&data)[N])
        : TParent(data, N)
    {
    }

    IdT operator[](const TStringBuf& str) const {
        return this->Name2Id(str);
    }

    const StrT& operator[](IdT id) const {
        return this->Id2Name(id);
    }
};

template <class IdT = ui32, class StrT = TStringBuf> //IdT must be integer type
class TStringIdDictionary: public TStringIdDictionaryOperators<IdT, StrT, NNameIdDictionary::TSolidId2Str<IdT>> {
    using TParent = TStringIdDictionaryOperators<IdT, StrT, NNameIdDictionary::TSolidId2Str<IdT>>;

public:
    TStringIdDictionary(size_t poolSegSize = 0)
        : TParent(poolSegSize)
    {
    }

    template <size_t N>
    TStringIdDictionary(const std::pair<const char*, IdT> (&data)[N])
        : TParent(data, N)
    {
    }

    // constructor for enum mapping (id <-> name)
    // Note: pairs should start with id = 0 and be tightly ordered
    TStringIdDictionary(const std::pair<const char*, IdT>* data, size_t size)
        : TParent(data, size)
    {
    }

    TStringIdDictionary(const TVector<std::pair<const char*, IdT>>& data)
        : TParent(data.data(), data.size())
    {
    }
};

template <class IdT = ui32, class StrT = TStringBuf> //IdT must be integer type
class TStringSparseIdDictionary: public TStringIdDictionaryOperators<IdT, StrT, NNameIdDictionary::TSparseId2Str<IdT>> {
    using TParent = TStringIdDictionaryOperators<IdT, StrT, NNameIdDictionary::TSparseId2Str<IdT>>;

public:
    TStringSparseIdDictionary(size_t poolSegSize = 0)
        : TParent(poolSegSize)
    {
    }

    template <size_t N>
    TStringSparseIdDictionary(const std::pair<const char*, IdT> (&data)[N])
        : TParent(data, N)
    {
    }

    // constructor for enum mapping (id <-> name)
    TStringSparseIdDictionary(const std::pair<const char*, IdT>* data, size_t size)
        : TParent(data, size)
    {
    }

    TStringSparseIdDictionary(const TVector<std::pair<const char*, IdT>>& data)
        : TParent(data.data(), data.size())
    {
    }
};

template <class E, size_t B>
inline void SetEnumFlags(const TStringIdDictionary<E>& dict, TStringBuf optSpec,
                         std::bitset<B>& flags, bool allIfEmpty = true) {
    if (optSpec.empty()) {
        SetEnumFlagsForEmptySpec(flags, allIfEmpty);
    } else {
        flags.reset();
        for (const auto& it : StringSplitter(optSpec).Split(',')) {
            E e = dict[ToString(it.Token()).data()];
            flags.set(e);
        }
    }
}
