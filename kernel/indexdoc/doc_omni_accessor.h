#pragma once

#include <util/system/yassert.h>
#include <util/generic/vector.h>

#include <kernel/doom/offroad_struct_wad/struct_type.h>
#include <kernel/doom/offroad_struct_wad/doc_data_holder.h>
#include <kernel/doom/wad/wad_index_type.h>


template <class Io>
class TDocOmniIndexAccessor {
    friend class TDocOmniWadIndex;
    friend class TOmniPrinter;
    friend class TOmniAccessorFactory;
    using TSearcher = typename Io::TSearcher;

    using TIterator = typename TSearcher::TIterator;

public:
    using THit = typename TSearcher::THit;
    static constexpr NDoom::EWadIndexType IndexType = Io::IndexType;
    static constexpr bool IsFixedSizeStruct = (Io::StructType == NDoom::FixedSizeStructType);
    using TIo = Io;

private:
    TDocOmniIndexAccessor(const TSearcher* searcher)
        : Searcher_(searcher)
    {
    }

    TDocOmniIndexAccessor(const TSearcher* searcher, const TSearchDocDataHolder* docDataHolder)
        : Searcher_(searcher)
        , Iterator_(docDataHolder)
    {}
public:

    THit GetHit(ui32 docId) {
        Y_ASSERT(Searcher_);
        if (docId == LastDocId_) {
            return LastHit_;
        }
        LastDocId_ = docId;
        THit hit;
        if (Searcher_->Find(docId, &Iterator_) && Iterator_.ReadHit(&hit)) {
            LastHit_ = hit;
            return hit;
        }
        LastHit_ = DummyHit_;
        return DummyHit_;
    }

    bool IsInited() const {
        return !!Searcher_;
    }

public:
    TVector<char> CompatibilityBuffer;

private:
    const TSearcher* Searcher_ = nullptr;
    TIterator Iterator_;
    THit LastHit_ = THit();
    ui32 LastDocId_ = ui32(-1);
    THit DummyHit_ = THit();
};

