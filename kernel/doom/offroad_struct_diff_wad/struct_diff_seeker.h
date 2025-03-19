#pragma once

#include <library/cpp/offroad/custom/ui32_vectorizer.h>
#include <library/cpp/offroad/custom/ui64_vectorizer.h>

#include <library/cpp/offroad/flat/flat_searcher.h>

namespace NDoom {


template <class Key, class Vectorizer, class Data, class Offset, class OffsetVectorizer>
class TStructDiffSeeker {
public:
    using TKey = Key;
    using TData = Data;

    TStructDiffSeeker() {

    }

    TStructDiffSeeker(const TBlob& blob)
        : Searcher_(blob)
    {

    }

    void Reset(const TBlob& blob) {
        Searcher_.Reset(blob);
    }

    template <class Reader>
    bool LowerBound(const TKey& key, TKey* firstKey, const TData** firstData, Reader* reader) const {
        const size_t index = Searcher_.LowerBound(key);

        TKey lastKey = TKey();
        ui64 offset = 0;

        if (index > 0) {
            lastKey = Searcher_.ReadKey(index - 1);
            offset = Searcher_.ReadData(index - 1);
        }

        if (!reader->Seek(offset, lastKey)) {
            return false;
        }

        return reader->LowerBoundLocal(key, firstKey, firstData);
    }

private:
    NOffroad::TFlatSearcher<Key, Offset, Vectorizer, OffsetVectorizer> Searcher_;
};


} // namespace NDoom
