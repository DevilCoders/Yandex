#pragma once

#include "template_stuff.h"
#include "compatibility_mapping.h"

#include <kernel/doom/offroad_omni_wad/omni_io.h>
#include <library/cpp/deprecated/omni/read.h>
#include <util/folder/dirut.h>

class TOldOmniIndexObsoleteDiePlease {

    using TLegacyMapping = TTypeList<
        TTypeTag<NDoom::TOmniUrlIo, 0>,
        TTypeTag<NDoom::TOmniTitleIo, 1>,
        TTypeTag<NDoom::TOmniDssmLogDwellTimeBigramsEmbeddingRawIo, 2>,
        TTypeTag<NDoom::TOmniAnnRegStatsRawIo, 4>,
        TTypeTag<NDoom::TOmniDssmAggregatedAnnRegEmbeddingRawIo, 6>,
        TTypeTag<NDoom::TOmniDssmAnnCtrCompressedEmbeddingRawIo, 9>,
        TTypeTag<NDoom::TOmniDssmAnnXfDtShowWeightCompressedEmbedding1RawIo, 10>,
        TTypeTag<NDoom::TOmniDssmAnnXfDtShowWeightCompressedEmbedding2RawIo, 11>,
        TTypeTag<NDoom::TOmniDssmAnnXfDtShowWeightCompressedEmbedding3RawIo, 12>,
        TTypeTag<NDoom::TOmniDssmAnnXfDtShowWeightCompressedEmbedding4RawIo, 13>,
        TTypeTag<NDoom::TOmniDssmAnnXfDtShowWeightCompressedEmbedding5RawIo, 14>,
        TTypeTag<NDoom::TOmniDssmAnnXfDtShowOneCompressedEmbeddingRawIo, 15>,
        TTypeTag<NDoom::TOmniDssmAnnXfDtShowOneSeCompressedEmbeddingRawIo, 16>,
        TTypeTag<NDoom::TOmniDssmMainContentKeywordsEmbeddingRawIo, 17>,
        TTypeTag<NDoom::TOmniDssmAnnXfDtShowOneSeAmSsHardCompressedEmbeddingRawIo, 18>,
        TTypeTag<NDoom::TOmniDssmAnnSerpSimilarityHardSeCompressedEmbeddingRawIo, 19>,
        TTypeTag<NDoom::TOmniRecDssmSpyTitleDomainCompressedEmb12RawIo, 20>,
        TTypeTag<NDoom::TOmniRecCFSharpDomainRawIo, 21>,
        TTypeTag<NDoom::TOmniDssmBertDistillL2EmbeddingRawIo, 22>,
        TTypeTag<NDoom::TOmniDssmNavigationL2EmbeddingRawIo, 23>,
        TTypeTag<NDoom::TOmniDssmFullSplitBertEmbeddingRawIo, 24>,
        TTypeTag<NDoom::TOmniRelCanonicalTargetIo, 25>,
        TTypeTag<NDoom::TOmniDssmSinsigL2EmbeddingRawIo, 26>,
        TTypeTag<NDoom::TOmniReservedEmbeddingRawIo, 27>
    >;

protected:
    TOldOmniIndexObsoleteDiePlease()
        : UseOmniLegacy_(false)
    {}

    TOldOmniIndexObsoleteDiePlease(const TString& oldIndexPath, bool useThisLegacy)
        : UseOmniLegacy_(useThisLegacy)
    {
        if (UseOmniLegacy_) {
            Y_VERIFY(NFs::Exists(oldIndexPath));
            Reader_.Reset(new NOmni::TOmniReader(oldIndexPath));
            Root_ = Reader_->Root().GetByKey(0);
        }
    }

    template<class Io>
    int GetFieldNum(std::true_type) const {
        int fieldNum = GetFieldNum<Io>(std::false_type());
        if (fieldNum < 0)
            fieldNum = TTagOfTypeList<typename NIndexDoc::TCompatibilityLayers<Io>::TType, TLegacyMapping>::Tag;
        return fieldNum;
    }

    template<class Io>
    int GetFieldNum(std::false_type) const {
        return TTagOfType<Io, TLegacyMapping>::Tag;
    }

    template <class Accessor>
    auto GetField(const ui32 docId, Accessor* accessor) const {
        using TIo = typename Accessor::TIo;

        int fieldNum = GetFieldNum<TIo>(typename NIndexDoc::THasCompatibilityLayers<TIo>::TType());
        const auto& doc = Root_.GetByKey(docId);

        if (fieldNum >= 0 && doc.HasKey((size_t)fieldNum)) {
            doc.GetByKey((size_t)fieldNum).GetData(&accessor->CompatibilityBuffer);
        } else {
            accessor->CompatibilityBuffer.clear();
        }

        return decltype(accessor->GetHit(docId))(accessor->CompatibilityBuffer.data(), accessor->CompatibilityBuffer.size());
    }

    size_t Size() const {
        return Root_.GetSize();
    }

protected:
    bool UseOmniLegacy_ = false;

private:
    THolder<NOmni::TOmniReader> Reader_;
    NOmni::TOmniIterator Root_;
};

