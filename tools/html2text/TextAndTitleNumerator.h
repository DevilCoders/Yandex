#pragma once

#include "TextAndTitleSentences.h"

#include <library/cpp/numerator/numerate.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>

class TTextAndTitleNumerator : public INumeratorHandler {
private:
    bool IsInsideTitle;
    TUtf16String CurrentTextSentence;
    TUtf16String CurrentTitleSentence;
    TSimpleSharedPtr<TTextAndTitleSentences> Sentences;

public:
    TTextAndTitleNumerator();

    void Reset();
    TSimpleSharedPtr<TTextAndTitleSentences> GetSentences() const;

private:
    void OnToken(const TChar* token, size_t length);
    void OnSentenceEnd();

    void OnMoveInput(const THtmlChunk& /*chunk*/, const TZoneEntry* zone, const TNumerStat&) override;
    void OnTokenStart(const TWideToken& tok, const TNumerStat&) override;
    void OnSpaces(TBreakType type, const TChar* tok, unsigned len, const TNumerStat&) override;
    void OnTextStart(const IParsedDocProperties*) override;
    void OnTextEnd(const IParsedDocProperties*, const TNumerStat&) override;
};
