#pragma once

#include <kernel/url_text_analyzer/lib/public.h>
#include <kernel/url_text_analyzer/lib/opts.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/ptr.h>

namespace NUta {
    namespace NPrivate {
        class IUrlAnalyzerImpl;
    }

    class TLiteUrlAnalyzer: public IUrlTextAnalyzer {
    public:
        TLiteUrlAnalyzer(const TFastStrictOpts& options = TFastStrictOpts());
        ~TLiteUrlAnalyzer();
        TVector<TString> AnalyzeUrlUTF8(const TStringBuf& url) const override;

    private:
        THolder<NPrivate::IUrlAnalyzerImpl> Impl;
        TOpts Options;
    };

}
