#pragma once

#include "opts.h"
#include "impl.h"

namespace NUta {
    TVector<TString> AnalyzeUrlUTF8Impl(const THolder<NPrivate::IUrlAnalyzerImpl>& impl, const TOpts& options, const TStringBuf& url);
}
