#pragma once

#include <dict/recognize/queryrec/queryrecognizer.h>

#include <kernel/title_ranges/lib/title_ranges.h>
#include <kernel/remorph/matcher/matcher.h>

class TDocTitleRangesCalcer {
public:
    typedef TSimpleSharedPtr<TQueryRecognizer> TQueryRecognizerPtr;
    typedef NReMorph::TMatcherPtr TMatcherPtr;

public:
    TDocTitleRangesCalcer(TQueryRecognizerPtr queryRecognizer);
    ~TDocTitleRangesCalcer();

    bool CalcRanges(const TUtf16String& title, TDocTitleRanges& ranges, const TLangMask& mask = TLangMask()) const;

private:
    class TImpl;

private:
    THolder<TImpl> Impl;
};
