#pragma once

#include <kernel/snippets/sent_match/callback.h>

#include <util/generic/string.h>
#include <util/stream/output.h>

namespace NSnippets {
    class TFinalFactorsDump : public ISnippetsCallback {
    private:
        TString Result;
    public:
        void OnBestFinal(const TSnip& snip, bool isByLink) override;
        void GetExplanation(IOutputStream& out) const override {
            out << Result;
        }
    };
}
