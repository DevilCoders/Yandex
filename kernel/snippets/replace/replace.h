#pragma once

#include "replaceresult.h"

#include <kernel/snippets/markers/markers.h>

#include <kernel/tarc/docdescr/docdescr.h>

#include <library/cpp/charset/doccodes.h>
#include <library/cpp/langs/langs.h>
#include <util/generic/string.h>
#include <util/generic/ptr.h>
#include <util/memory/blob.h>

class TInlineHighlighter;

namespace NUrlCutter {
    class TRichTreeWanderer;
}

namespace NSnippets {

class TReplaceManager;
class TQueryy;
class TSnipTitle;
class TSnip;
class TSentsMatchInfo;
class TMetaDescription;
class TConfig;
class TLengthChooser;
class TArchiveMarkup;
class TWordSpanLen;
class TCustomSnippetsStorage;
class TExtraSnipAttrs;
struct ISnippetsCallback;
class TTopCandidateCallback;

class IReplacer {
protected:
    const TString ReplacerName;
    IReplacer(const TString& replacerName)
        : ReplacerName(replacerName)
    {
    }
public:
    virtual void DoWork(TReplaceManager* manager) = 0;
    virtual ~IReplacer()
    {
    }
    const TString& GetReplacerName() const {
        return ReplacerName;
    }
};

class TReplaceContext : private TNonCopyable
{
public:
    const TQueryy& QueryCtx;
    const TSentsMatchInfo& SentsMInfo;
    const TSnip& Snip;

    const TSnipTitle& NaturalTitle;
    const TUtf16String& NaturalTitleSource; //without cutting and additions
    const TSnipTitle& SuperNaturalTitle;

    const ECharset DocEncoding;
    const TDocInfos& DocInfos;
    const TString& Url;
    const TMetaDescription& MetaDescr;
    const TInlineHighlighter& IH;
    NUrlCutter::TRichTreeWanderer& RichtreeWanderer;

    const bool IsByLink;
    const ELanguage DocLangId;
    const bool IsNav;

    const TConfig& Cfg;
    const TLengthChooser& LenCfg;
    const TArchiveMarkup& Markup;
    const TWordSpanLen& SnipWordSpanLen;
    const TSnip& OneFragmentSnip;

public:
    TReplaceContext(
        const TQueryy& qctx,
        const TSentsMatchInfo& info,
        const TSnip& snip,
        const TSnipTitle& naturalTitle,
        const TUtf16String& naturalTitleSource,
        const TSnipTitle& superNaturalTitle,
        ECharset docEncoding,
        const TDocInfos& docinfos,
        const TString& url,
        const TMetaDescription& metaDescr,
        const TInlineHighlighter& ih,
        NUrlCutter::TRichTreeWanderer& richtreeWanderer,
        bool bylink,
        const ELanguage langId,
        bool isNav,
        const TConfig& cfg,
        const TLengthChooser& lenCfg,
        const TArchiveMarkup& markup,
        const TWordSpanLen& snipWordSpanLen,
        const TSnip& oneFragmentSnip
    )
        : QueryCtx(qctx)
        , SentsMInfo(info)
        , Snip(snip)
        , NaturalTitle(naturalTitle)
        , NaturalTitleSource(naturalTitleSource)
        , SuperNaturalTitle(superNaturalTitle)
        , DocEncoding(docEncoding)
        , DocInfos(docinfos)
        , Url(url)
        , MetaDescr(metaDescr)
        , IH(ih)
        , RichtreeWanderer(richtreeWanderer)
        , IsByLink(bylink)
        , DocLangId(langId)
        , IsNav(isNav)
        , Cfg(cfg)
        , LenCfg(lenCfg)
        , Markup(markup)
        , SnipWordSpanLen(snipWordSpanLen)
        , OneFragmentSnip(oneFragmentSnip)
    {
    }
};

class TReplaceManager {
private:
    class TImpl;

    THolder<TImpl> Impl;

private:
    void ReportReplacerName();

public:
    TReplaceManager(TAutoPtr<const TReplaceContext> repCtx, TExtraSnipAttrs& specAttrs, ISnippetsCallback& callback, TTopCandidateCallback* fsCallback);
    ~TReplaceManager();

    ISnippetsCallback& GetCallback();
    TTopCandidateCallback* GetFactSnippetTopCandidatesCallback();

    TExtraSnipAttrs& GetExtraSnipAttrs();
    const TReplaceResult& GetResult() const;
    const TReplaceContext& GetContext() const;
    TCustomSnippetsStorage& GetCustomSnippets();

    void AddReplacer(TAutoPtr<IReplacer> replacer);
    void AddPostReplacer(TAutoPtr<IReplacer> postReplacer);
    void DoWork();
    bool IsReplaced() const;
    bool IsBodyReplaced() const;
    void Commit(const TReplaceResult& result, const EMarker marker = MRK_COUNT);
    void ReplacerDebug(const TString& comment);
    void ReplacerDebug(const TString& comment, const TReplaceResult& result);
    const TString& GetReplacerUsed();
    void SetMarker(const EMarker marker, bool value = true);
};


}
