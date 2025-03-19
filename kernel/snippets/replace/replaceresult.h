#pragma once

#include <kernel/snippets/smartcut/multi_length_cut.h>
#include <kernel/snippets/titles/make_title/util_title.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/ptr.h>

namespace NSnippets {

class TSnip;
class TTabSeparatedAttrs;
class TConfig;
class TQueryy;

class TReplaceResult {
private:
    class TImpl;
    THolder<TImpl> Impl;

public:
    TReplaceResult();
    TReplaceResult(const TReplaceResult& other);
    ~TReplaceResult();
    TReplaceResult& operator=(const TReplaceResult& other);

    const TTabSeparatedAttrs& GetSpecSnipAttrs() const;
    void AppendSpecSnipAttr(TStringBuf name, TStringBuf value);
    void ClearSpecSnipAttrs();
    const TSnipTitle* GetTitle() const;
    const TSnip* GetSnip() const;
    TReplaceResult& UseText(const TUtf16String& headline, const TString& src);
    TReplaceResult& UseText(const TMultiCutResult& headlineWithExt, const TString& src);
    TReplaceResult& UseSnip(const TSnip& theSnip, const TString& src);
    TReplaceResult& UseTitle(const TSnipTitle& title);
    TReplaceResult& SetPreserveSnip();
    bool CanUse() const;
    const TString& GetTextSrc() const;
    void SetTextSrc(const TString& src);
    const TUtf16String& GetText() const;
    const TMultiCutResult& GetTextExt() const;
    bool HasCustomSnip() const;
};

TSnipTitle MakeSpecialTitle(const TUtf16String& source, const TConfig& cfg, const TQueryy& query);

}
