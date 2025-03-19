#pragma once

#include <ysite/yandex/pure/pure.h>
#include <library/cpp/charset/doccodes.h>

class TPureWithCachedWord {
public:
    TPureWithCachedWord(const TPure& pure, const TUtf16String& word);

    const TPureRecord& GetRecord() const { return Record; }
    ui64 GetByLex(const TUtf16String& w, NPure::ECase caseFlags, ELanguage language = LANG_UNK) const;
    ui64 GetByForm(const TUtf16String& w, NPure::ECase caseFlags, ELanguage language = LANG_UNK) const;
    const TLangMask& LemmatizedLanguages() const { return Pure.LemmatizedLanguages(); }

private:
    const TPure& Pure;
    TUtf16String Word;
    TPureRecord Record;
};
