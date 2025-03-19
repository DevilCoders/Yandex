#pragma once

#include <library/cpp/langs/langs.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>

struct TZonedString;

namespace NSnippets {

class TDecapitalizer {
private:
    class TImpl;

    THolder<TImpl> Impl;

public:
    TDecapitalizer(const TUtf16String& sent, ELanguage lang);
    ~TDecapitalizer();

    void DecapitalSentence(const wchar16 *pc, const size_t len);
    void DecapitalSingleWord(const wchar16 *pc, const size_t len, bool needFirstUpper = true);
    void RevertFioChanges(const wchar16 *pc, const size_t len);
    void Complete(TZonedString &z);
    TUtf16String GetSent() const;
};

void DecapitalSentence(TUtf16String &w, ELanguage lang);

}

