#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/ptr.h>

namespace NUta::NPrivate {
    class IUrlTokenHandler;

    class IUrlAnalyzerImpl {
    public:
        virtual bool TryUntranslit(const TUtf16String& word, TUtf16String* result, ui32 maxTranslitCandidates) const = 0;
        virtual TVector<TUtf16String> FindSplit(const TUtf16String& word,
            bool penalizeForLength,
            bool tryUntranslit,
            ui32 minSubTokenLen,
            ui32 maxTranslitCandidates) const = 0;
        virtual ~IUrlAnalyzerImpl() = default;
    };
}
