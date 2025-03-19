#pragma once

#include <kernel/doom/offroad_struct_diff_wad/offroad_struct_diff_wad_io.h>
#include <library/cpp/offroad/custom/subtractors.h>

#include "kernel/doom/enums/accessor_state.h"

#include <kernel/doom/standard_models/standard_models.h>

#include <util/stream/output.h>

class TRegHostErfInfo;

namespace NDoom {

    class TStructDiffKey {
    public:
        TStructDiffKey() = default;

        TStructDiffKey(ui32 docId, ui32 region)
            : DocId_(docId)
            , Region_(region)
        {

        }

        ui32 DocId() const {
            return DocId_;
        }

        ui32 Region() const {
            return Region_;
        }

        friend bool operator==(const TStructDiffKey& l, const TStructDiffKey& r) {
            return std::pair<ui32, ui32>(l.DocId_, l.Region_) == std::pair<ui32, ui32>(r.DocId_, r.Region_);
        }

        friend bool operator>=(const TStructDiffKey& l, const TStructDiffKey& r) {
            return std::pair<ui32, ui32>(l.DocId_, l.Region_) >= std::pair<ui32, ui32>(r.DocId_, r.Region_);
        }

    private:
        ui32 DocId_ = 0;
        ui32 Region_ = 0;
    };

    struct TStructDiffKeyVectorizer {
        enum {
            TupleSize = 2
        };

        template <class Slice>
        Y_FORCE_INLINE static void Scatter(const TStructDiffKey& key, Slice&& slice) {
            slice[0] = key.DocId();
            slice[1] = key.Region();
        }

        template <class Slice>
        Y_FORCE_INLINE static void Gather(Slice&& slice, TStructDiffKey* key) {
            *key = TStructDiffKey(slice[0], slice[1]);
        }
    };

    using TStructDiffKeySubtractor = NOffroad::TD2Subtractor;


    template <class ErfType, EWadIndexType indexType, EStandardIoModel defaultModel>
    using TRegErfGeneralIo = TOffroadStructDiffWadIo<indexType, TStructDiffKey, TStructDiffKeyVectorizer, TStructDiffKeySubtractor, TStructDiffKeyVectorizer, ErfType, defaultModel>;
    using TRegHostErfIo = TRegErfGeneralIo<TRegHostErfInfo, RegHostErfIndexType, DefaultRegHostErfIoModel>;


template <class Io>
class TGeneralRegErfAccessor {
    using TSearcher = typename Io::TSearcher;
    using TIterator = typename TSearcher::TIterator;
    using THit = typename TSearcher::TData;


public:
    using TRegErf = std::remove_pointer_t<THit>;


    TGeneralRegErfAccessor(EAccessorState)
    {
    }

    TGeneralRegErfAccessor(const TSearcher* searcher)
        : Searcher_(searcher)
    {
    }
public:
    const THit* GetRegErf(ui32 docId, ui32 region) {
        const THit* hit;
        typename Io::TKey key(docId, region);
        typename Io::TKey firstKey;
        if (Searcher_->LowerBound(key, &firstKey, &hit, &Iterator_) && key == firstKey) {
            return hit;
        }
        return nullptr;
    }

private:
    const TSearcher* Searcher_ = nullptr;
    TIterator Iterator_;
};

} //namespace NDoom

Y_DECLARE_OUT_SPEC(inline, NDoom::TStructDiffKey, stream, value) {
    stream << value.DocId() << ":" << value.Region();
}

using TRegHostErfAccessor = NDoom::TGeneralRegErfAccessor<NDoom::TRegHostErfIo>;
