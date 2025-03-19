#pragma once

#include <kernel/snippets/sent_match/tsnip.h>

#include <kernel/snippets/archive/staticattr.h>
#include <kernel/snippets/idl/raw_preview.pb.h>
#include <kernel/snippets/markers/markers.h>
#include <kernel/snippets/smartcut/multi_length_cut.h>

#include <library/cpp/charset/wide.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/map.h>

namespace NSnippets {

class TLinkAttrs {
private:
    TString Urls;

public:
    void AppendLink(const TString& Url) {
        if (Urls.size()) {
            Urls += "\x07;";
        }
        Urls.append("url=");
        Urls.append(Url);
    }
    const TString& GetLinkAttrs() const {
        return Urls;
    }
};

class TTabSeparatedAttrs {
private:
    TString Attr;

public:
    void Append(TStringBuf name, TStringBuf value) {
        if (Attr.size()) {
            Attr.append("\t");
        }
        Attr.append(name);
        Attr.append('\t');
        Attr.append(value);
    }

    const TString& GetPacked() const {
        return Attr;
    }

    void Clear() {
        Attr.clear();
    }
};

class TExtraSnipAttrs {
private:
    TLinkAttrs LinkAttrs;
    TTabSeparatedAttrs SpecAttrs;
    TString PreviewJson;
    TMap<TString, TString> ClickLikeSnip;
    NProto::TRawPreview RawPreview;
    TSnip ExtendedSnippet;
    TMultiCutResult ExtendedHeadline;
    TString DocQuerySig;
    TString DocStaticSig;
    TString EntityClassifyResult;
    TString SchemaVthumb;
    TString Urlmenu;
    TString HilitedUrl;

public:
    TMarkersMask Markers;

public:
    void AppendLinkAttr(const TString& url) {
        LinkAttrs.AppendLink(url);
    }

    const TString& GetLinkAttrs() const {
        return LinkAttrs.GetLinkAttrs();
    }

    void SetSpecAttrs(const TTabSeparatedAttrs& value) {
        SpecAttrs = value;
    }

    void AppendSpecAttr(TStringBuf name, TStringBuf value) {
        SpecAttrs.Append(name, value);
    }

    void SetPreviewJson(const TString& s) {
        PreviewJson = s;
    }

    const TString& GetPreviewJson() const {
        return PreviewJson;
    }

    void SetRawPreview(const NProto::TRawPreview& rawPreview) {
        RawPreview = rawPreview;
    }

    const NProto::TRawPreview& GetRawPreview() const {
        return RawPreview;
    }

    void SetExtendedSnippet(const TSnip& extSnip) {
        ExtendedSnippet = extSnip;
    }

    const TSnip& GetExtendedSnippet() const {
        return ExtendedSnippet;
    }

    void SetExtendedHeadline(const TMultiCutResult& extHeadline) {
        ExtendedHeadline = extHeadline;
    }

    const TMultiCutResult& GetExtendedHeadline() const {
        return ExtendedHeadline;
    }

    const TString& GetSpecAttrs() const {
        return SpecAttrs.GetPacked();
    }

    void AddClickLikeSnipJson(const TString& key, const TString& jsonValue) {
        ClickLikeSnip[key] = jsonValue;
    }
    TString GetPackedClickLikeSnip() const;

    void SetDocQuerySig(const TString& value) {
        DocQuerySig = value;
    }
    const TString& GetDocQuerySig() const {
        return DocQuerySig;
    }

    void SetDocStaticSig(const TString& value) {
        DocStaticSig = value;
    }
    const TString& GetDocStaticSig() const {
        return DocStaticSig;
    }

    void SetEntityClassifyResult(const TString& value) {
        EntityClassifyResult = value;
    }
    const TString& GetEntityClassifyResult() const {
        return EntityClassifyResult;
    }

    void SetSchemaVthumb(const TString& value) {
        SchemaVthumb = value;
    }
    const TString& GetSchemaVthumb() const {
        return SchemaVthumb;
    }

    void SetUrlmenu(const TString& value) {
        Urlmenu = value;
    }
    const TString& GetUrlmenu() const {
        return Urlmenu;
    }

    void SetHilitedUrl(const TString& value) {
        HilitedUrl = value;
    }
    const TString& GetHilitedUrl() const {
        return HilitedUrl;
    }
};

}
