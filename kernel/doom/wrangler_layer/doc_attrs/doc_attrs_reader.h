#pragma once

#include <kernel/doom/offroad_doc_wad/offroad_doc_wad_io.h>

#include <kernel/doom/hits/doc_attrs_hit.h>
#include <kernel/doom/offroad_doc_attrs_wad/doc_attrs_hit_adaptors.h>
#include <kernel/groupattrs/docsattrs.h>

namespace NDoom {

using TDocAttrsIo = TOffroadDocWadIo<DocAttrsIndexType, TDocAttrsHit, TDocAttrsHitVectorizer, TDocAttrsHitSubtractor, TDocAttrsHitPrefixVectorizer, NoStandardIoModel>;
using TDocAttrs64Io = TOffroadDocWadIo<DocAttrsIndexType, TDocAttrs64Hit, TDocAttrs64HitVectorizer, TDocAttrs64HitSubtractor, TDocAttrsHitPrefixVectorizer, NoStandardIoModel>;


template<typename TWadIo>
class TDocAttrsReader: private TNonCopyable {
public:
    using THit = typename TWadIo::TReader::THit;

    TDocAttrsReader(const TString &path)
        : DocAttrs_(false, path.c_str(), false)
    {
        Size_ = DocAttrs_.DocCount();
        AttrCount_ = DocAttrs_.Config().AttrCount();
    }

    void Restart() {
        CurDoc_ = 0;
        CurHit_ = 0;
    }

    bool ReadHit(THit* hit) {
        if (CurHit_ < Hits_.size()) {
            *hit = Hits_[CurHit_++];
            return true;
        } else {
            return false;
        }
    }

    bool ReadDoc(ui32* docId) {
        if (CurDoc_ < Size_) {
            *docId = CurDoc_;
            Hits_.clear();
            TCategSeries categs;
            for (ui32 i = 0; i < AttrCount_; i++) {
                DocAttrs_.DocCategs(CurDoc_, i, categs, nullptr);
                for (size_t j = 0; j < categs.size(); j++) {
                    Hits_.push_back(THit(i, categs.GetCateg(j)));
                }
            }

            CurHit_ = 0;
            CurDoc_++;
            return true;
        } else {
            return false;
        }
    }

    TProgress Progress() const {
        return TProgress(CurDoc_, Size_);
    }

private:
    NGroupingAttrs::TDocsAttrs DocAttrs_;
    ui32 CurDoc_ = 0;
    size_t CurHit_ = 0;
    ui32 Size_ = 0;
    ui32 AttrCount_ = 0;
    TVector<THit> Hits_;
};

} // namespace NDoom
