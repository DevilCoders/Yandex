#include "raw_preview_fillers.h"

#include <kernel/snippets/idl/raw_preview.pb.h>

namespace NSnippets {
    struct TPreviewItemFiller::TImpl {
        NProto::TPreviewItem* PreviewItem;

        TImpl(NProto::TPreviewItem* previewItem)
            : PreviewItem(previewItem)
        {}
    };

    TPreviewItemFiller::TPreviewItemFiller(NProto::TPreviewItem* const previewItem)
        : Impl(new TImpl(previewItem))
    {}

    TPreviewItemFiller::TPreviewItemFiller(const TPreviewItemFiller& filler)
        : Impl(new TImpl(filler.Impl.Get()->PreviewItem))
    {}

    TPreviewItemFiller::~TPreviewItemFiller() {
    }

    void TPreviewItemFiller::SetName(const TString& value) {
        if (Impl->PreviewItem != nullptr)
            Impl->PreviewItem->SetName(value);
    }

    void TPreviewItemFiller::SetDescription(const TString& value) {
        if (Impl->PreviewItem != nullptr)
            Impl->PreviewItem->SetDescription(value);
    }

    void TPreviewItemFiller::SetUrl(const TString& value) {
        if (Impl->PreviewItem != nullptr)
            Impl->PreviewItem->SetUrl(value);
    }

    void TPreviewItemFiller::AddImage(const TString& url) {
        if (Impl->PreviewItem != nullptr)
            Impl->PreviewItem->AddImages()->SetUrl(url);
    }

    void TPreviewItemFiller::AddProperty(const TString& key, const TString& value, const bool isMain) {
        if (Impl->PreviewItem != nullptr) {
            NProto::TProperty* property = Impl->PreviewItem->AddProperties();
            property->SetKey(key);
            property->SetValue(value);
            property->SetIsMain(isMain);
        }
    }

    struct TRawPreviewFiller::TImpl {
        NProto::TRawPreview RawPreview;
        bool NeedToBeFilled;

        TImpl(const bool needToBeFilled)
            : NeedToBeFilled(needToBeFilled)
        {}
    };

    TRawPreviewFiller::TRawPreviewFiller(const bool needToBeFilled)
        : Impl(new TImpl(needToBeFilled))
    {}

    TRawPreviewFiller::~TRawPreviewFiller() {
    }

    const NProto::TRawPreview& TRawPreviewFiller::GetRawPreview() const {
        return Impl->RawPreview;
    }

    void TRawPreviewFiller::SetResultsCountInAll(const ui32 value) {
        if (Impl->NeedToBeFilled)
            Impl->RawPreview.SetResultsCountInAll(value);
    }

    void TRawPreviewFiller::SetTemplate(const TString& value) {
        if (Impl->NeedToBeFilled)
            Impl->RawPreview.SetTemplate(value);
    }

    void TRawPreviewFiller::SetRawPreview(const TRawPreviewFiller& rawPreviewFiller) {
        if (Impl->NeedToBeFilled)
            Impl->RawPreview = rawPreviewFiller.GetRawPreview();
    }

    TPreviewItemFiller TRawPreviewFiller::GetPreviewItemFiller() {
        if (Impl->NeedToBeFilled)
            return TPreviewItemFiller(Impl->RawPreview.AddPreviewItems());
        return TPreviewItemFiller();
    }
}
