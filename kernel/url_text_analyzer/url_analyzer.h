#pragma once

#include <kernel/url_text_analyzer/lib/public.h>
#include <kernel/url_text_analyzer/lib/opts.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/ptr.h>

namespace NUta {
    class TSmartUrlAnalyzer : public IUrlTextAnalyzer {
    public:
        TSmartUrlAnalyzer(const TVector<TString>& pureFiles, const TOpts& options = TDefaultSmartSplitOpts());
        TSmartUrlAnalyzer(const TFastStrictOpts& options = TFastStrictOpts());
        TSmartUrlAnalyzer(const TLightExperimentalOpts& options);
        ~TSmartUrlAnalyzer();

        TVector<TString> AnalyzeUrlUTF8(const TStringBuf& url) const override;

    private:
        THolder<NPrivate::IUrlAnalyzerImpl> Impl;
        TOpts Options;

        TSmartUrlAnalyzer(const TOpts&) = delete;
    };

} // NUta
