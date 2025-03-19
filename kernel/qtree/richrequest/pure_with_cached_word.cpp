#include "pure_with_cached_word.h"

TPureWithCachedWord::TPureWithCachedWord(const TPure& pure, const TUtf16String& word)
    : Pure(pure)
    , Word(word)
    , Record(pure.GetRecord(word))
{
}

ui64 TPureWithCachedWord::GetByForm(const TUtf16String& w, NPure::ECase caseFlags, ELanguage language) const {
    if (w == Word) {
        return Record.GetByForm(caseFlags, language);
    } else {
        return Pure.GetRecord(w).GetByForm(caseFlags, language);
    }
}

ui64 TPureWithCachedWord::GetByLex(const TUtf16String& w, NPure::ECase caseFlags, ELanguage language) const {
    if (w == Word) {
        return Record.GetByLex(caseFlags, language);
    } else {
        return Pure.GetRecord(w).GetByLex(caseFlags, language);
    }
}
