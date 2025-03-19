#pragma once

#include <util/generic/fwd.h>
#include <util/generic/ptr.h>

namespace NSnippets {
    namespace NProto {
        class TPreviewItem;
        class TRawPreview;
    }

    class TPreviewItemFiller {
    private:
        struct TImpl;
        THolder<TImpl> Impl;

    public:
        TPreviewItemFiller(NProto::TPreviewItem* const previewItem = nullptr);
        TPreviewItemFiller(const TPreviewItemFiller& filler);
        ~TPreviewItemFiller();


        void SetName(const TString& value);
        void SetDescription(const TString& value);
        void SetUrl(const TString& value);
        void AddImage(const TString& url);
        void AddProperty(const TString& key, const TString& value, const bool isMain);
    };

    class TRawPreviewFiller {
    private:
        struct TImpl;
        THolder<TImpl> Impl;

    public:
        TRawPreviewFiller(const bool needToBeFilled);
        ~TRawPreviewFiller();

        const NProto::TRawPreview& GetRawPreview() const;
        void SetResultsCountInAll(const ui32 value);
        void SetTemplate(const TString& value);
        void SetRawPreview(const TRawPreviewFiller& rawPreviewFiller);
        TPreviewItemFiller GetPreviewItemFiller();
    };
}
