#pragma once

#include "segmentator.h"

#include <library/cpp/numerator/numerate.h>

namespace NSegm {

template<typename TStorer = IStorer>
class TSegmentatorHandler: public INumeratorHandler {
protected:
    NPrivate::TSegmentator Segmentator;
    TStorer Storer;
    THolder<NPrivate::TSegContext> CtxHolder;

public:
    typedef TStorer TInternalStorer;

    // Use html5Parser = true only with library/cpp/html/html5 parser running before numerator
    TSegmentatorHandler(bool html5Parser = false)
    {
        Segmentator.SetStorer(&Storer);
        Segmentator.SetHtml5Parser(html5Parser);
    }

    const TInternalStorer& GetStorer() const {
        return Storer;
    }

    TInternalStorer& GetStorer() {
        return Storer;
    }

    void InitSegmentator(const char* url, const TOwnerCanonizer* c, NPrivate::TSegContext* ctx = nullptr) {
        if (!ctx) {
            CtxHolder.Reset(new NPrivate::TSegContext);
            Segmentator.SetSegContext(CtxHolder.Get());
        } else {
            Segmentator.SetSegContext(ctx);
        }
        Segmentator.InitOwnerInfo(url, c);
    }

    const THashTokens& GetTokens() const {
        return Segmentator.GetTokens();
    }

    const TString& GetUrl() const {
        return Segmentator.GetUrl();
    }

    void SetSkipMainContent() {
        Segmentator.SetSkipMainContent();
    }

    void SetSkipMainHeader() {
        Segmentator.SetSkipMainHeader();
    }

    const TDocContext& GetDocContext() const {
        return Segmentator.GetDocContext();
    }

    TDocContext& GetDocContext() {
        return Segmentator.GetDocContext();
    }

    const TArticleSpans& GetArticleSpans() const {
        return Segmentator.GetArticleSpans();
    }

    TArticleSpans& GetArticleSpans() {
        return Segmentator.GetArticleSpans();
    }

    TMainHeaderSpans& GetMainHeaderSpans() {
        return Segmentator.GetMainHeaderSpans();
    }

    const TMainHeaderSpans& GetMainHeaderSpans() const {
        return Segmentator.GetMainHeaderSpans();
    }

    const THeaderSpans& GetHeaderSpans() const {
        return Segmentator.GetHeaderSpans();
    }

    THeaderSpans& GetHeaderSpans() {
        return Segmentator.GetHeaderSpans();
    }

    const THeaderSpans& GetStrictHeaderSpans() const {
        return Segmentator.GetStrictHeaderSpans();
    }

    THeaderSpans& GetStrictHeaderSpans() {
        return Segmentator.GetStrictHeaderSpans();
    }

    TMainContentSpans& GetMainContentSpans() {
        return Segmentator.GetMainContentSpans();
    }

    const TMainContentSpans& GetMainContentSpans() const {
        return Segmentator.GetMainContentSpans();
    }

    const TTypedSpans& GetListSpans() const {
        return Segmentator.GetListSpans();
    }

    TTypedSpans& GetListSpans() {
        return Segmentator.GetListSpans();
    }

    const TTypedSpans& GetListItemSpans() const {
        return Segmentator.GetListItemSpans();
    }

    TTypedSpans& GetListItemSpans() {
        return Segmentator.GetListItemSpans();
    }

    const TSpans& GetTableSpans() const {
        return Segmentator.GetTableSpans();
    }

    TSpans& GetTableSpans() {
        return Segmentator.GetTableSpans();
    }

    const TSpans& GetTableRowSpans() const {
        return Segmentator.GetTableRowSpans();
    }

    TSpans& GetTableRowSpans() {
        return Segmentator.GetTableRowSpans();
    }

    const TSpans& GetTableCellSpans() const {
        return Segmentator.GetTableCellSpans();
    }

    TSpans& GetTableCellSpans() {
        return Segmentator.GetTableCellSpans();
    }

    const TSegmentSpans& GetSegmentSpans() const {
        return Segmentator.GetSegmentSpans();
    }

    TSegmentSpans& GetSegmentSpans() {
        return Segmentator.GetSegmentSpans();
    }

    const TUrlInfo& GetOwnerInfo() const {
        return Segmentator.GetOwnerInfo();
    }

    TUrlInfo& GetOwnerInfo() {
        return Segmentator.GetOwnerInfo();
    }

    void OnSpaces(TBreakType t, const wchar16* s, unsigned l, const TNumerStat& stat) override {
        Segmentator.ProcessSpaces(t, s, l, stat);
        bool title = Segmentator.IsTitleText(stat);
        if (title || !Segmentator.IgnoreContent())
            Storer.OnSpaces(title, t, s, l, TAlignedPosting(stat.TokenPos.Pos));
    }

    void OnTokenStart(const TWideToken& wtok, const TNumerStat& stat) override {
        if (Segmentator.ProcessToken(wtok, stat))
            Storer.OnToken(Segmentator.IsTitleText(stat), wtok, TAlignedPosting(stat.TokenPos.Pos));
    }

    void OnTextStart(const IParsedDocProperties* p) override {
        Storer.SetParser(p);
    }

    void OnTextEnd(const IParsedDocProperties*, const TNumerStat& stat) override {
        Segmentator.ProcessTextEnd(stat);
        Storer.OnTextEnd(TAlignedPosting(stat.TokenPos.Pos));
    }

    void OnMoveInput(const THtmlChunk& chunk, const TZoneEntry* ze, const TNumerStat& stat) override {
        Segmentator.BeforeProcessDiscardInput(stat);
        Segmentator.ProcessDiscardInput(chunk, ze, stat);
        Segmentator.AfterProcessDiscardInput();
        Storer.OnEvent(Segmentator.IsTitleText(stat), chunk, ze, TAlignedPosting(stat.TokenPos.Pos));
    }

};

}
