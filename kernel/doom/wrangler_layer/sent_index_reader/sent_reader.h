#pragma once

#include <kernel/sent_lens/sent_lens.h>
#include <kernel/doom/progress/progress.h>
#include <kernel/doom/hits/sent_len_hit.h>
#include <type_traits>

namespace NDoom {

class TSentIndexReader: private TNonCopyable {
public:
    using THit = TSentLenHit;

    TSentIndexReader(const TString& path)
        : SentReader_(path)
        , Size_(SentReader_.GetSize())
    {
        Restart();
    }

    void Restart() {
        CurDoc_ = 0;
        CurrLengths_.clear();
        CurLengthIdx_ = 0;
    }

    bool ReadHit(THit* hit) {
        if (CurrLengths_.Size() <= CurLengthIdx_)
            return false;

        hit->SetLength(CurrLengths_[CurLengthIdx_]);
        hit->SetDocId(CurDoc_ - 1);
        CurLengthIdx_++;
        return true;
    }

    bool ReadDoc(ui32* docId) {
        if (!Advance())
            return false;

        *docId = CurDoc_ - 1;
        CurrLengths_.clear();
        SentReader_.Get(*docId, &CurrLengths_);
        CurLengthIdx_ = 0;

        return true;
    }

    TProgress Progress() const {
        return TProgress(CurDoc_, Size_);
    }

    size_t Size() const {
        return Size_;
    }

private:
    bool Advance() {
        CurDoc_++;
        return CurDoc_ != Size_ + 1;
    }

private:
    TSentenceLengthsReader SentReader_;
    ui32 CurDoc_ = 0;
    TSentenceLengths CurrLengths_;
    ui32 CurLengthIdx_ = 0;
    ui32 Size_ = 0;
};

} //namespace NDoom
