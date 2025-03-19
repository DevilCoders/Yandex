#include "url_analyzer.h"

#include <kernel/url_text_analyzer/lib/analyze.h>
#include <util/generic/yexception.h>

namespace NUta {
    using namespace NPrivate;

    class TLiteUrlAnalyzerImpl: public IUrlAnalyzerImpl {
        bool TryUntranslit(const TUtf16String&, TUtf16String*, ui32) const override {
            ythrow yexception() << "use TSmartUrlAnalyzer for TryUntranslit";
            return {};
        }
        TVector<TUtf16String> FindSplit(const TUtf16String&, bool, bool, ui32, ui32) const override {
            return {};
        }
    };

    TLiteUrlAnalyzer::TLiteUrlAnalyzer(const TFastStrictOpts& fastOptions)
        : Options(fastOptions)
    {
        Impl.Reset(new TLiteUrlAnalyzerImpl());
    }

    TLiteUrlAnalyzer::~TLiteUrlAnalyzer() = default;

    TVector<TString> TLiteUrlAnalyzer::AnalyzeUrlUTF8(const TStringBuf& url) const {
        return AnalyzeUrlUTF8Impl(Impl, Options, url);
    }
}
