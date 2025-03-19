#pragma once

#include <kernel/snippets/sent_match/callback.h>

#include <kernel/snippets/strhl/goodwrds.h>

#include <util/generic/ptr.h>

namespace NSnippets
{
    class TConfig;
    class TExtraSnipAttrs;

    class TTopCandidateCallback : public ISnippetsCallback
    {
    private:
      class TImpl;
      THolder<TImpl> Impl;

    public:
      TTopCandidateCallback(const TConfig& /*cfg*/, TExtraSnipAttrs& /*extraSnipAttrs*/, const TInlineHighlighter& /* ih */, bool explainMode);
      ~TTopCandidateCallback() override;

      ISnippetCandidateDebugHandler* GetCandidateHandler() override;
      void GetExplanation(IOutputStream& expl) const override;
      void OnTitleSnip(const TSnip& /*natural*/, const TSnip& unnatural, bool /*isByLink*/) override;
      void OnBestFinal(const TSnip& /*snip*/, bool /*isByLink*/) override;
      void OnPassageReply(const TPassageReply& reply, const TEnhanceSnippetConfig& cfg) override;
      TVector<TSnip>& GetBestCandidates();
    };

}
