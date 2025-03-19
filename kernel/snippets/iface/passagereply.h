#pragma once

#include <kernel/snippets/idl/secondary_snippets.pb.h>
#include <kernel/snippets/idl/raw_preview.pb.h>

#include <util/generic/vector.h>
#include <util/generic/string.h>

namespace NMetaProtocol {
    class TArchiveInfo;
}

namespace NSnippets {

extern const TUtf16String EV_NOTITLE_WIDE;
extern const TUtf16String EV_NOHEADLINE_WIDE;

class TPassageReplyData {
public:
    bool LinkSnippet = false;
    TVector<TUtf16String> Passages;
    TVector<TString> Attrs;
    TUtf16String Title;
    TUtf16String Headline;
    TString HeadlineSrc;
    size_t SnipLengthInSymbols = 0;
    float SnipLengthInRows = 0.0f;
    TString SnippetsExplanation;
    TString SpecSnippetAttrs;
    TString LinkAttrs;
    TString DocQuerySig;
    TString DocStaticSig;
    TString EntityClassifyResult;
    TString ImagesJson;
    TString ImagesDups;
    TString ImgDupsAttrs; //will be deprecated some day
    TString PreviewJson;
    NProto::TRawPreview RawPreview;
    TVector<TString> Markers;
    TString AltSnippetsPacked;
    TString SchemaVthumb;
    TString UrlMenu;
    TString HilitedUrl;
    TString ClickLikeSnip; // convert doc attrs to click server response format
};

class TPassageReply {
private:
    bool HadError = false;
    TString ErrorMessage;
    TPassageReplyData Data;

public:
    bool GetError() const {
        return HadError;
    }

    const TString& GetErrorMessage() const {
        return ErrorMessage;
    }

    int GetPassagesType() const noexcept {
        return Data.LinkSnippet ? 1 : 0;
    }

    const TVector<TUtf16String>& GetPassages() const noexcept {
        return Data.Passages;
    }

    const TVector<TString>& GetPassagesAttrs() const noexcept {
        return Data.Attrs;
    }

    const TUtf16String& GetTitle() const noexcept {
        return Data.Title;
    }

    const TUtf16String& GetHeadline() const noexcept {
        return Data.Headline;
    }

    const TString& GetHeadlineSrc() const noexcept {
        return Data.HeadlineSrc;
    }

    float GetSnipLengthInRows() const {
        return Data.SnipLengthInRows;
    }

    size_t GetSnipLengthInSymbols() const {
        return Data.SnipLengthInSymbols;
    }

    const TString& GetSnippetsExplanation() const noexcept {
        return Data.SnippetsExplanation;
    }

    const TString& GetSpecSnippetAttrs() const noexcept {
        return Data.SpecSnippetAttrs;
    }

    const TString& GetLinkAttrs() const noexcept {
        return Data.LinkAttrs;
    }

    const TString& GetDocQuerySig() const noexcept {
        return Data.DocQuerySig;
    }

    const TString& GetEntityClassifyResult() const noexcept {
        return Data.EntityClassifyResult;
    }

    const TString& GetDocStaticSig() const noexcept {
        return Data.DocStaticSig;
    }

    const TString& GetAltSnippets() const noexcept {
        return Data.AltSnippetsPacked;
    }

    const TString& GetUrlMenu() const noexcept {
        return Data.UrlMenu;
    }

    void SetUrlMenu(const TString& urlmenu) {
        Data.UrlMenu = urlmenu;
    }

    const TString& GetHilitedUrl() const noexcept {
        return Data.HilitedUrl;
    }

    void SetHilitedUrl(const TString& hilitedurl) {
        Data.HilitedUrl = hilitedurl;
    }

    const TString& GetClickLikeSnip() const noexcept {
        return Data.ClickLikeSnip;
    }

    void SetClickLikeSnip(const TString& clickLikeSnip) {
        Data.ClickLikeSnip = clickLikeSnip;
    }

    TPassageReply() {
    }

    // For use with internal tools;
    // reconstruct from the meta response
    explicit TPassageReply(const NMetaProtocol::TArchiveInfo& ai);

    void Reset() {
        *this = TPassageReply();
    }

    void Set(const TPassageReplyData& data);

    void SetError();
    void SetError(const TString& error);

    void SetExplanation(const TString& snippetsExplanation) {
        Data.SnippetsExplanation = snippetsExplanation;
    }

    void SetImagesJson(const TString& value) {
        Data.ImagesJson = value;
    }
    const TString& GetImagesJson() const {
        return Data.ImagesJson;
    }
    void SetImagesDups(const TString& value) {
        Data.ImagesDups = value;
    }
    const TString& GetImagesDups() const {
        return Data.ImagesDups;
    }
    void SetImgDupsAttrs(const TString& value) {
        Data.ImgDupsAttrs = value;
    }
    const TString& GetImgDupsAttrs() const { //will be deprecated some time
        return Data.ImgDupsAttrs;
    }
    void AddMarker(const TString& value) {
        Data.Markers.push_back(value);
    }
    size_t GetMarkersCount() const {
        return Data.Markers.size();
    }

    void PackAltSnippets(NSnippets::NProto::TSecondarySnippets& ss);

    void PackToArchiveInfo(NMetaProtocol::TArchiveInfo& destination) const;

    const TString& GetMarker(size_t i) const {
        return Data.Markers[i];
    }
    void ClearPassagesAttrs() {
        Data.Attrs.clear();
    }

    void SetPreviewJson(const TString& value) {
        Data.PreviewJson = value;
    }

    const TString& GetPreviewJson() const {
        return Data.PreviewJson;
    }

    void SetRawPreview(const NProto::TRawPreview& value) {
        Data.RawPreview = value;
    }

    const NProto::TRawPreview& GetRawPreview() const {
        return Data.RawPreview;
    }

    const TString& GetSchemaVthumb() const {
        return Data.SchemaVthumb;
    }

    void SetSchemaVthumb(const TString& value) {
        Data.SchemaVthumb = value;
    }

    // FIXME temp functions, don't forget to delete them (SNIPPETS-2174, SNIPPETS-2749)
    void SetPassages(const TVector<TUtf16String>& v) {
        Data.Passages = v;
    }
    void SetTitle(const TUtf16String& wtr) {
        Data.Title = wtr;
    }
    void SetHeadline(const TUtf16String& wtr) {
        Data.Headline = wtr;
    }

};

}
