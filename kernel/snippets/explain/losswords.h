#pragma once

#include <kernel/snippets/sent_match/callback.h>

#include <util/generic/string.h>
#include <util/generic/ptr.h>

namespace NSnippets {
    class TConfig;
    class TSnip;
    class TArchiveMarkup;

    class TLossWordsCallback: public ISnippetsCallback {
    private:
        class TImpl;
        THolder<TImpl> Impl;

    public:
        TLossWordsCallback(const TArchiveMarkup& markup, const TConfig& cfg, const TString& url);
        ~TLossWordsCallback() override;
        ISnippetTextDebugHandler* GetTextHandler(bool isBylink) override;
        ISnippetCandidateDebugHandler* GetCandidateHandler() override;
        void GetExplanation(IOutputStream& output) const override;
        void OnTitleSnip(const TSnip& natural, const TSnip& unnatural, bool isByLink) override;
        void OnBestFinal(const TSnip& snip, bool isByLink) override;
        void OnTitleReplace(bool replace) override;
    };

    class TLossWordsExplain {
    private:
        class TImpl;
        TImpl* Impl;

    public:
        TLossWordsExplain(IOutputStream& out);
        ~TLossWordsExplain();
        void Parse(const TString& explanation);
        void Print();
    };
}
