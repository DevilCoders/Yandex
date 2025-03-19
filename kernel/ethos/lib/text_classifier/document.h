#pragma once

#include <kernel/ethos/lib/data/dataset.h>

#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/ysaveload.h>

namespace NEthos {

struct TDocument {
    TVector<ui64> UnigramHashes;

    TDocument() {}

    TDocument(const TDocument&) = default;

    TDocument(TDocument&& document)
        : UnigramHashes(std::move(document.UnigramHashes))
    {
    }

    TDocument(const TVector<ui64>& unigramHashes)
        : UnigramHashes(unigramHashes)
    {
    }

    TDocument(TVector<ui64>&& unigramHashes)
        : UnigramHashes(std::move(unigramHashes))
    {
    }

    TDocument& operator=(const TDocument&) = default;
};

using TDocuments = TVector<TDocument>;

struct TMultiLabelDocument: TDocument {
    ui32 Index = 0;
    TString Id;
    TVector<TString> Labels;
    float Weight = 1.f;

    Y_SAVELOAD_DEFINE(Index, Id, Labels, UnigramHashes, Weight);

    TMultiLabelDocument() {}

    TMultiLabelDocument(const TMultiLabelDocument&) = default;

    TMultiLabelDocument(TMultiLabelDocument&& document)
        : TDocument(std::move(document))
        , Index(document.Index)
        , Id(std::move(document.Id))
        , Labels(std::move(document.Labels))
        , Weight(document.Weight)
    {
    }

    TMultiLabelDocument(TDocument&& document,
                        const ui32 index,
                        TString&& id,
                        TVector<TString>&& labels,
                        float weight = 1.f)
        : TDocument(std::move(document))
        , Index(index)
        , Id(std::move(id))
        , Labels(std::move(labels))
        , Weight(weight)
    {
    }

    TMultiLabelDocument(TVector<ui64>&& unigramHashes,
                        const ui32 index,
                        TString&& id,
                        TVector<TString>&& labels,
                        float weight = 1.f)
        : TDocument(std::move(unigramHashes))
        , Index(index)
        , Id(std::move(id))
        , Labels(std::move(labels))
        , Weight(weight)
    {
    }

    TMultiLabelDocument& operator=(const TMultiLabelDocument&) = default;
};

using TMultiLabelDocuments = TVector<TMultiLabelDocument>;

struct TBinaryLabelDocument: TDocument {
    ui32 Index = 0;
    TString Id;
    EBinaryClassLabel Label = EBinaryClassLabel::BCL_UNKNOWN;
    float Weight = 1.f;

    Y_SAVELOAD_DEFINE(Index, Id, Label, UnigramHashes, Weight);

    TBinaryLabelDocument() {}

    TBinaryLabelDocument(const TBinaryLabelDocument&) = default;

    TBinaryLabelDocument(TBinaryLabelDocument&& document)
        : TDocument(std::move(document))
        , Index(document.Index)
        , Id(std::move(document.Id))
        , Label(document.Label)
        , Weight(document.Weight)
    {
    }

    TBinaryLabelDocument(TMultiLabelDocument&& document, const TString& targetLabel)
        : TDocument(std::move(document.UnigramHashes))
        , Index(document.Index)
        , Id(std::move(document.Id))
        , Weight(document.Weight)
    {
        Label = ::IsIn(document.Labels, targetLabel)
                ? EBinaryClassLabel::BCL_POSITIVE
                : EBinaryClassLabel::BCL_NEGATIVE;
    }

    TBinaryLabelDocument(TDocument&& document,
                         const ui32 index,
                         TString&& id,
                         EBinaryClassLabel label,
                         float weight = 1.f)
        : TDocument(std::move(document))
        , Index(index)
        , Id(std::move(id))
        , Label(label)
        , Weight(weight)
    {
    }

    TBinaryLabelDocument(TVector<ui64>&& unigramHashes,
                         const ui32 index,
                         TString&& id,
                         EBinaryClassLabel label,
                         float weight = 1.f)
        : TDocument(std::move(unigramHashes))
        , Index(index)
        , Id(std::move(id))
        , Label(label)
        , Weight(weight)
    {
    }

    TBinaryLabelDocument& operator=(const TBinaryLabelDocument&) = default;
};

using TBinaryLabelDocuments = TVector<TBinaryLabelDocument>;

}
