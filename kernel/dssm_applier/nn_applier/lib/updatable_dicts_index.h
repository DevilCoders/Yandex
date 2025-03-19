#pragma once

#include "blob_hash_set.h"

#include <util/string/cast.h>
#include <util/digest/murmur.h>
#include <util/charset/wide.h>

namespace NNeuralNetApplier {

enum class EStoreStringType {
    UTF16,
    UTF8,
};

template <class T>
class TUpdatableDictIndex {
public:
    TUpdatableDictIndex() = default;

    TUpdatableDictIndex(const TVector<T>& hashes, T startId, T hashModulo, EStoreStringType stringType = EStoreStringType::UTF16, bool allowCollisions = false)
        : BlobHashSet_(TBlobHashSet<T>::FromContainer(hashes))
        , HashModulo_(hashModulo)
        , StartId_(startId)
        , StringType_(stringType)
        , AllowCollisions_(allowCollisions)
    {
    }

    void Save(IOutputStream* os) const {
        TStringStream ss;
        THashMap<TString, TString> fields;
        fields["HashModulo"] = ToString(HashModulo_);
        fields["StartId"] = ToString(StartId_);
        fields["StringType"] = ToString(StringType_);
        fields["AllowCollisions"] = ToString(AllowCollisions_);
        SaveFields64(&ss, fields);
        BlobHashSet_.Save(&ss);
        SaveString64(os, ss.Str());
    }

    void Init(TBlob& blob) {
        TBlob curBlob = ReadBlob(blob);
        const THashMap<TString, TString> fields = ReadFields(curBlob);
        HashModulo_ = FromString<T>(fields.at("HashModulo"));
        StartId_ = FromString<T>(fields.at("StartId"));
        if (auto it = fields.find("StringType"); it != fields.end()) {
            StringType_ = FromString<EStoreStringType>(it->second);
        }
        if (auto it = fields.find("AllowCollisions"); it != fields.end()) {
            AllowCollisions_ = FromString<bool>(it->second);
        }
        BlobHashSet_.Load(curBlob);
    }

    bool Find(const TWtringBuf& w, size_t* index) const {
        const T hash = CalcHash(w, StringType_);
        if (Y_LIKELY(AllowCollisions_) || BlobHashSet_.Contains(hash)) {
            if (Y_LIKELY(HashModulo_)) {
                *index = StartId_ + hash % HashModulo_;
            } else {
                *index = StartId_ + hash;
            }
            return true;
        }
        return false;
    }

    static T CalcHash(const TWtringBuf& wideStr, EStoreStringType stringType) {
        if (stringType == EStoreStringType::UTF16) {
            static_assert(std::is_same<TWtringBuf::TChar, TUtf16String::TChar>::value);
            return MurmurHash<T>(wideStr.data(), wideStr.size() * sizeof(TWtringBuf::TChar));
        }
        const TString utfStr = WideToUTF8(wideStr);
        return MurmurHash<T>(utfStr.data(), utfStr.size());
    }

    const TBlobHashSet<T>& GetHashSet() const {
        return BlobHashSet_;
    }

private:
    TBlobHashSet<T> BlobHashSet_;
    T HashModulo_;
    T StartId_ = 0;
    /**
     * It is better to use UTF16, because nn_applier's tokenizers call Find(const TWtringBuf& w, size_t* index)
     * so you can escape unnecessary UTF8 <-> UTF16 transform
     */
    EStoreStringType StringType_ = EStoreStringType::UTF16;
    bool AllowCollisions_ = false;
};

}  // namespace NNeuralNetApplier
