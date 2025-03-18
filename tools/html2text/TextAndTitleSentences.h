#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>

const TUtf16String WideNewLine = (TUtf16String) (wchar16)'\n';
const TUtf16String WideSpace = (TUtf16String) (wchar16)' ';
const TUtf16String WideTab = (TUtf16String) (wchar16)'\t';

class TTextAndTitleSentences {
public:
    TVector<TUtf16String> TitleSentences;
    TVector<TUtf16String> TextSentences;

    TUtf16String GetText(TUtf16String delimiter = WideSpace) const;
    TUtf16String GetTitle(TUtf16String delimiter = WideSpace) const;
};
