#include "replaceresult.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/sent_match/extra_attrs.h>

#include <kernel/snippets/smartcut/clearchar.h>
#include <kernel/snippets/smartcut/multi_length_cut.h>

#include <kernel/snippets/titles/make_title/make_title.h>

namespace NSnippets {

    class TReplaceResult::TImpl {
    public:
        TMultiCutResult Headline;
        TString HeadlineSrc;
        THolder<TSnip> Snip;
        THolder<TSnipTitle> Title;
        TTabSeparatedAttrs SpecSnipAttrs;
        bool PreserveSnip = false;

        TImpl() {
        }

        TImpl(const TImpl& other)
            : Headline(other.Headline)
            , HeadlineSrc(other.HeadlineSrc)
            , Snip(other.Snip ? new TSnip(*other.Snip.Get()) : nullptr)
            , Title(other.Title ? new TSnipTitle(*other.Title.Get()) : nullptr)
            , SpecSnipAttrs(other.SpecSnipAttrs)
            , PreserveSnip(other.PreserveSnip)
        {
        }
    };

    TReplaceResult::TReplaceResult()
      : Impl(new TImpl())
    {
    }
    TReplaceResult::TReplaceResult(const TReplaceResult& other)
      : Impl(new TImpl(*other.Impl.Get()))
    {
    }
    TReplaceResult::~TReplaceResult()
    {
    }
    TReplaceResult& TReplaceResult::operator=(const TReplaceResult& other)
    {
        Impl.Reset(new TImpl(*other.Impl.Get()));
        return *this;
    }

    const TTabSeparatedAttrs& TReplaceResult::GetSpecSnipAttrs() const {
        return Impl->SpecSnipAttrs;
    }

    void TReplaceResult::AppendSpecSnipAttr(TStringBuf name, TStringBuf value) {
        Impl->SpecSnipAttrs.Append(name, value);
    }

    void TReplaceResult::ClearSpecSnipAttrs() {
        Impl->SpecSnipAttrs.Clear();
    }

    const TSnipTitle* TReplaceResult::GetTitle() const {
        return Impl->Title.Get();
    }

    static const TSnip EMPTY_SNIP = TSnip();

    const TSnip* TReplaceResult::GetSnip() const {
        if (Impl->Snip) {
            return Impl->Snip.Get();
        }
        if ((Impl->Headline || Impl->HeadlineSrc) && !Impl->PreserveSnip) {
            return &EMPTY_SNIP;
        }
        return nullptr;
    }

    TReplaceResult& TReplaceResult::UseText(const TUtf16String& headline, const TString& src) {
        Impl->Headline = TMultiCutResult(headline);
        Impl->HeadlineSrc = src;
        return *this;
    }

    TReplaceResult& TReplaceResult::UseText(const TMultiCutResult& headlineWithExt, const TString& src) {
        Impl->Headline = headlineWithExt;
        Impl->HeadlineSrc = src;
        return *this;
    }

    TReplaceResult& TReplaceResult::UseSnip(const TSnip& theSnip, const TString& src) {
        Impl->Snip.Reset(new TSnip(theSnip));
        Impl->HeadlineSrc = src;
        return *this;
    }

    TReplaceResult& TReplaceResult::UseTitle(const TSnipTitle& title) {
        Impl->Title.Reset(new TSnipTitle(title));
        return *this;
    }

    TReplaceResult& TReplaceResult::SetPreserveSnip() {
        Impl->PreserveSnip = true;
        return *this;
    }

    bool TReplaceResult::CanUse() const {
        return Impl->HeadlineSrc.size();
    }

    const TString& TReplaceResult::GetTextSrc() const {
        return Impl->HeadlineSrc;
    }

    void TReplaceResult::SetTextSrc(const TString& src) {
        Impl->HeadlineSrc = src;
    }

    const TUtf16String& TReplaceResult::GetText() const {
        return Impl->Headline.Short;
    }

    const TMultiCutResult& TReplaceResult::GetTextExt() const {
        return Impl->Headline;
    }

    bool TReplaceResult::HasCustomSnip() const {
        return !!Impl->Snip;
    }

    TSnipTitle MakeSpecialTitle(const TUtf16String& source, const TConfig& cfg, const TQueryy& query) {
        TMakeTitleOptions options(cfg);
        options.DefinitionMode = TDM_IGNORE;
        TUtf16String cleanSource = source;
        ClearChars(cleanSource, /* allowSlash */ false, cfg.AllowBreveInTitle());
        return MakeTitle(cleanSource, cfg, query, options);
    }
}
