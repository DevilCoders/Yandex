#pragma once

#include <library/cpp/langmask/langmask.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NInfl {

struct TSimpleResultInfo {
    TString Grams;
    TString Debug;

    explicit TSimpleResultInfo()
        : Grams()
        , Debug()
    {
    }
};

class TSimpleInflector {
private:
    TLangMask Langs;
    TUtf16String OutputDelimiter;

public:
    explicit TSimpleInflector(const TString& langs = TString());

    TUtf16String Inflect(const TUtf16String& text, const TString& grams, TSimpleResultInfo* resultInfo = nullptr, bool pluralization = false) const;
};

} // NInfl
