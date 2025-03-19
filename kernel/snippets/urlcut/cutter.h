#pragma once

#include "tokens.h"
#include <kernel/snippets/custom/hostnames_data/hostnames.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NUrlCutter {
    class THilitedString;

    class TUrlCutter {
    private:
        class TUrlCutterImpl;
        THolder<TUrlCutterImpl> Impl;
    public:
        TUrlCutter(TTokenList& tokens, const NSnippets::W2WTrie& domainTrie, i32 maxLen = 76, i32 thresholdLen = 45);
        ~TUrlCutter();
        THilitedString GetUrl();
    };
}
