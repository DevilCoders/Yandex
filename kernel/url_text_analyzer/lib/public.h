#pragma once

#include <util/generic/fwd.h>

namespace NUta::NPrivate {
    class IUrlAnalyzerImpl;
}

namespace NUta {
    class IUrlTextAnalyzer {
    public:
        virtual TVector<TString> AnalyzeUrlUTF8(const TStringBuf& url) const = 0;
        virtual ~IUrlTextAnalyzer() = default;
    };
}
