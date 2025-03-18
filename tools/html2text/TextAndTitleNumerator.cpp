#include "TextAndTitleNumerator.h"

TTextAndTitleNumerator::TTextAndTitleNumerator() :
    IsInsideTitle(false),
    CurrentTextSentence(),
    CurrentTitleSentence(),
    Sentences(nullptr)
{ }

void TTextAndTitleNumerator::Reset() {
    IsInsideTitle = false;
    Sentences.Reset(new TTextAndTitleSentences);
    CurrentTextSentence.clear();
    CurrentTitleSentence.clear();
}

TSimpleSharedPtr<TTextAndTitleSentences> TTextAndTitleNumerator::GetSentences() const {
    return Sentences;
}

void TTextAndTitleNumerator::OnToken(const TChar* token, size_t length) {
    CurrentTextSentence.append(token, length);
    if (IsInsideTitle) {
        CurrentTitleSentence.append(token, length);
    }
}

void TTextAndTitleNumerator::OnSentenceEnd() {
    if (CurrentTextSentence.length() > 0) {
        Sentences->TextSentences.push_back(CurrentTextSentence);
        CurrentTextSentence.clear();
    }
    if (CurrentTitleSentence.length() > 0) {
        Sentences->TitleSentences.push_back(CurrentTitleSentence);
        CurrentTitleSentence.clear();
    }
}

void TTextAndTitleNumerator::OnMoveInput(const THtmlChunk& /*chunk*/, const TZoneEntry* zone, const TNumerStat&) {
    if (zone && !zone->OnlyAttrs && zone->Name && strcmp("title", zone->Name) == 0) {
        IsInsideTitle = zone->IsOpen;
    }
}

void TTextAndTitleNumerator::OnTokenStart(const TWideToken& tok, const TNumerStat&) {
    OnToken(tok.Token, tok.Leng);
}

void TTextAndTitleNumerator::OnSpaces(TBreakType type, const TChar* tok, unsigned len, const TNumerStat&) {
    if (len != 0) {
        OnToken(tok, len);

        if (IsSentBrk(type)) {
            OnSentenceEnd();
        }
    }
}

void TTextAndTitleNumerator::OnTextStart(const IParsedDocProperties*) {
    Reset();
}

void TTextAndTitleNumerator::OnTextEnd(const IParsedDocProperties*, const TNumerStat&) {
    OnSentenceEnd();
}
