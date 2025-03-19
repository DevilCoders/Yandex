#pragma once

#include "blob_array.h"

#include <util/generic/vector.h>
#include <util/generic/hash.h>

namespace NNeuralNetApplier {

namespace NDefaultTraits {

template <class T, class HashFnc = THash<T>>
struct TDefaultTraits {
    static constexpr T EmptyMarker = Max<T>();

    static size_t CalcHash(const T& t) {
        return HashFnc()(t);
    }
};

}  // NDefaultTraits


/**
 * @brief HashSet that supports mapping from memory, Save, Load; Does not support delete;
 */
template <class T, class Traits = NDefaultTraits::TDefaultTraits<T>>
class TBlobHashSet {
private:
    TVector<T> DataHolder_;  // holds data while building
    TBlobArray<T> HashTable_;  // holds data when Loaded, refers to DataHolder_ in case of building TBlobHashSet

    static constexpr double DefaultScaleFactor = 4.0 / 3.0;

public:
    TBlobHashSet() = default;

    explicit TBlobHashSet(size_t dataSize, double scaleFactor = DefaultScaleFactor)
        : DataHolder_(Max<size_t>(2, dataSize * scaleFactor), Traits::EmptyMarker)
        , HashTable_(TBlobArray<T>::NoCopy(DataHolder_))
    {
        Y_ENSURE(
            DataHolder_.size() >= dataSize,
            DataHolder_.size() << " = dataSize * scaleFactor should be greater than dataSize = " << dataSize);
    }

    template<class TContainer>
    static TBlobHashSet<T> FromContainer(const TContainer& container, double scaleFactor = DefaultScaleFactor) {
        TBlobHashSet<T> builder(container.size(), scaleFactor);
        for (const T& key : container) {
            builder.Add(key);
        }
        return builder;
    }

    bool Contains(const T& key) const {
        if (key == Traits::EmptyMarker) {
            return false;
        }
        size_t it = FindFirstKeyOrEmpty(key);
        return HashTable_[it] == key;
    }

    void Save(IOutputStream* os) const {
        HashTable_.Save(os);
    }

    size_t Load(const TBlob& data) {
        Y_ASSERT(DataHolder_.empty());
        size_t read = HashTable_.Load(data);
        return read;
    }

private:
    void MoveIter(size_t& it) const {
        ++it;
        if (it == HashTable_.size()) {
            it = 0;
        }
    }

    size_t CalcHash(const T& key) const {
        return Traits::CalcHash(key) % HashTable_.size();
    }

    size_t FindFirstKeyOrEmpty(const T& key) const {
        size_t firstIt = CalcHash(key);
        size_t it = firstIt;
        while (HashTable_[it] != Traits::EmptyMarker && HashTable_[it] != key) {
            MoveIter(it);
            Y_ASSERT(it != firstIt);
        }
        return it;
    }

    void Add(T key) {
        if (key == Traits::EmptyMarker) {
            return;
        }
        size_t it = FindFirstKeyOrEmpty(key);
        if (DataHolder_[it] == Traits::EmptyMarker) {
            DataHolder_[it] = std::move(key);
        }
    }
};

}  // namespace NNeuralNetApplier
