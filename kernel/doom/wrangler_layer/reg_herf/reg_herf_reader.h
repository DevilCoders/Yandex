#pragma once

#include <type_traits>
#include <kernel/relevstatic/calc_regstatic.h>
#include <kernel/doom/progress/progress.h>
#include "reg_herf_wad_index_io.h"


namespace NDoom {

template <class RegErfIo>
class TRegErfReader: private TNonCopyable {
public:
    using THit = typename RegErfIo::TWriter::TIndexHit;

    using TRegHostErfType = typename RegErfIo::TData;

    TRegErfReader(const TString& path)
        : HErf_(path)
        , Size_(HErf_.GetSize())
    {
        Restart();
    }

    void Restart() {
        CurDoc_ = 0;
    }

    bool ReadHit(THit* hit) {
        if (CurrentKeyIdx_ >= CurrentKeys_.size()) {
            return false;
        }
        hit->Hit = &HErf_.Get(CurDoc_ - 1, CurrentKeys_[CurrentKeyIdx_]);
        hit->Region = CurrentKeys_[CurrentKeyIdx_];
        hit->DocId = CurDoc_ - 1;
        CurrentKeyIdx_++;
        return true;
    }

    size_t Size() {
        return Size_;
    }

    bool ReadDoc(ui32* docId) {
        if (!Advance())
            return false;
        *docId = CurDoc_ - 1;
        HErf_.GetKeys(CurrentKeys_, *docId);
        Sort(CurrentKeys_);
        CurrentKeyIdx_ = 0;
        return true;
    }

    TProgress Progress() const {
        return TProgress(CurDoc_, Size_);
    }

private:
    bool Advance() {
        CurDoc_++;
        return CurDoc_ != Size_ + 1;
    }

private:
    TArrayWithHead2D<TRegHostErfType> HErf_;
    ui32 CurDoc_ = 0;
    TVector<ui64> CurrentKeys_;
    size_t CurrentKeyIdx_ = 0;
    ui32 Size_ = 0;
};

} //namespace NDoom
