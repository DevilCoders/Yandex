#pragma once

#include <kernel/doom/offroad_struct_wad/offroad_struct_wad_io.h>
#include <kernel/doom/offroad_struct_wad/serializers.h>
#include <kernel/doom/enums/accessor_state.h>

class SDocErf2Info;
namespace NDoom {
    template <class ErfType, EWadIndexType indexType, EStandardIoModel model>
    using TErfGeneralIo = TOffroadStructWadIo<indexType, const ErfType*, TErfSerializer<const ErfType*>, FixedSizeStructType, OffroadCompressionType, model>;
    using TErf2Io = TErfGeneralIo<SDocErf2Info, ErfIndexType, DefaultErfIoModel>;
} // namespace NDoom

template <class ErfIo>
class TGeneralErfAccessor {
    template <class TDocErfType, class TDocErfIo>
    friend class TCustomIoDocErfManager;
    friend class THostErfManager;
    friend class TErfAccessorFactory;

    using TSearcher = typename ErfIo::TSearcher;
    using TIterator = typename TSearcher::TIterator;
    using THit = typename TSearcher::THit;
public:
    using TErf = std::remove_const_t<std::remove_pointer_t<THit>>;

    TGeneralErfAccessor(TSearcher* searcher)
        : Searcher_(searcher)
    {
    }

    TGeneralErfAccessor(TSearcher* searcher, const TSearchDocDataHolder* docDataHolder)
        : Searcher_(searcher)
        , Iterator_(docDataHolder)
    {}

    bool IsInitilized() const {
        return Searcher_;
    }

    void EnableRemoteErf(std::function<bool(const TErf&, const TErf&)> cmp = {}, std::function<void(const TErf&, TErf&)> merge = {}) {
        Cmp_ = cmp;
        Merge_ = merge;
        TryRemoteErf_ = true;
        LastDocId_ = ui32(-1);
    }

    const TErf& GetErf(ui32 docId) {
        Y_ASSERT(Searcher_);
        if (docId == LastDocId_) {
            return *LastErf_;
        }
        LastDocId_ = docId;
        THit hit;

        LastErf_ = nullptr;

        if (TryRemoteErf_) {
            if (Searcher_->Find(docId, &Iterator_, /*useWadLumps*/false) && Iterator_.ReadHit(&hit)) {
                RemoteErf_ = *hit;
                LastErf_ = &RemoteErf_;
            }
        }

        if (Searcher_->Find(docId, &Iterator_) && Iterator_.ReadHit(&hit)) {
            if (LastErf_) {
                if (Merge_) {
                    Merge_(*hit, RemoteErf_);
                }

                if (!Cmp_ || Cmp_(*LastErf_, *hit)) {
                    return *LastErf_;
                }
            }

            LastErf_ = hit;
            return *hit;
        }

        //return trash here
        LastErf_ = &DummyHit_;
        return DummyHit_;
    }
private:
    TGeneralErfAccessor(NDoom::EAccessorState)
    {
    }

private:
    TSearcher* Searcher_ = nullptr;
    TIterator Iterator_;
    TErf DummyHit_ = TErf();
    ui32 LastDocId_ = ui32(-1);
    THit LastErf_ = nullptr;

    bool TryRemoteErf_ = false;
    std::function<bool(const TErf& lhs, const TErf& rhs)> Cmp_;
    std::function<void(const TErf& local, TErf& remote)> Merge_;
    TErf RemoteErf_;
};

using TBasesearchErfAccessor = TGeneralErfAccessor<NDoom::TErf2Io>;
