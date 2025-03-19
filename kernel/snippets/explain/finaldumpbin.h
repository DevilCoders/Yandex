#pragma once

#include <kernel/snippets/sent_match/callback.h>

#include <kernel/snippets/config/config.h>

#include <util/generic/string.h>
#include <util/stream/output.h>

namespace NSnippets {
    class TFinalFactorsDumpBinary : public ISnippetsCallback {
    private:
        TString Result;
        TFactorsToDump FactorsToDump;
    public:
        TFinalFactorsDumpBinary(const TFactorsToDump& factorsToDump)
            : FactorsToDump(factorsToDump)
        {
        }
        void OnBestFinal(const TSnip& snip, bool isByLink) override;
        void GetExplanation(IOutputStream& out) const override {
            out << Result;
        }
        static TString DumpFactors(const TSnip& snip, const TFactorsToDump& factorsToDump);
    };
}
