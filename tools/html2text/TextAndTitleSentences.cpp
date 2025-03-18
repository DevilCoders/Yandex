#include "TextAndTitleSentences.h"

#include <util/string/vector.h>

TUtf16String TTextAndTitleSentences::GetText(TUtf16String delimiter) const {
    return JoinStrings(TextSentences, delimiter);
}

TUtf16String TTextAndTitleSentences::GetTitle(TUtf16String delimiter) const {
    return JoinStrings(TitleSentences, delimiter);
}
