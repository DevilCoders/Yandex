#include "directtokenizer.h"

#include <kernel/indexer/dtcreator/dtcreator.h>
#include <kernel/indexer/direct_text/dt.h>

namespace NIndexerCore {

TDirectTokenizer::TDirectTokenizer(TDirectTextCreator& creator, bool backwardCompatible, const TWordPosition& startPos)
    : Creator(creator)
    , BackwardCompatible(backwardCompatible)
    , OldLanguageOptions(LANG_UNK)
{
    PosOK = true;
    CurrentPos = startPos;
    WordCount = 0;
    LemmerOptions = 0;
    IgnoreStoreTextBreaks = false;
    IgnoreStoreTextNextWord = false;
    NoImplicitBreaks = false;
    StoreTextBreaks = 0;
    StoreTextMaxBreaks = BREAK_LEVEL_Max;
}

void TDirectTokenizer::AddDoc(ui32 docId, ELanguage lang) {
    CurrentPos = TWordPosition(0, 1, 1);
    PosOK = true;
    WordCount = 0;
    Creator.ClearDirectText();
    Creator.AddDoc(docId, lang);
}

void TDirectTokenizer::SetLangOptions(const TLangMask& langMask, ELanguage lang, ui8 lemmerOptions) {
    TIndexLanguageOptions langOptions(langMask, lang);
    OldLanguageOptions = Creator.SetLanguageOptions(langOptions);
    LemmerOptions = lemmerOptions;
}

void TDirectTokenizer::ResetLangOptions() {
    Creator.SetLanguageOptions(OldLanguageOptions);
    LemmerOptions = 0;
}

void TDirectTokenizer::CommitDoc() {
    Creator.CommitDoc();
}

void TDirectTokenizer::IncBreak(ui32 k) {
    OnBreak();
    CurrentPos.SetBreak(CurrentPos.Break() + k);
    CurrentPos.SetWord(TWordPosition::FIRST_CHILD);

    const static wchar16 dummySpace = 0x0020;
    while (k--)
        OnSpaces(NLP_SENTBREAK, &dummySpace, 1);
}

inline bool TDirectTokenizer::NextBreak() {
    if (CurrentPos.Word() == TWordPosition::FIRST_CHILD)
        return false;
    if (NoImplicitBreaks)
        ythrow TAllDoneException();
    OnBreak();
    PosOK = CurrentPos.Bump();
    return true;
}

inline void TDirectTokenizer::OnTokenStart(const TWideToken& tok) {
    if (PosOK) {
        Creator.StoreForm(tok, 0, CurrentPos.DocLength(), LemmerOptions);
    }
}

inline void TDirectTokenizer::OnSpaces(NLP_TYPE type, const wchar16* token, size_t len) {
    if (PosOK)
        Creator.StoreSpaces(token, (ui32)len, GetSpaceType(type));
}

void TDirectTokenizer::OnToken(const TWideToken& tok, size_t /*origleng*/, NLP_TYPE type) {
    Y_ASSERT(tok.Token && *tok.Token && tok.Leng);
    switch (type) {
        case NLP_WORD:
        case NLP_MARK:
        case NLP_INTEGER:
        case NLP_FLOAT:
        {
            size_t subTokenCount = tok.SubTokens.size();
            Y_ASSERT(subTokenCount >= 1);

            if (IgnoreSpecialKeys && tok.Leng == 1) {
                wchar32 c32 = ReadSymbol(tok.Token, tok.Token + tok.Leng);
                if (IsSpecialTokenizerSymbol(c32)) {
                    return;
                }
            }

            if (subTokenCount > 1) {
                // do not set break inside multitoken
                if (CurrentPos.Word() + subTokenCount - 1 > WORD_LEVEL_Max) {
                    if (NextBreak())
                        CheckStoreTextBreaks();
                }
            }

            if (RelevLevelCallback) {
                CurrentPos.SetRelevLevel(RelevLevelCallback->OnToken(type, tok));
            }

            OnTokenStart(tok);

            if (!IgnoreStoreTextNextWord) {
                Y_ASSERT(subTokenCount <= WORD_LEVEL_Max);
                while (subTokenCount--)
                    NextWord();
                if (CurrentPos.Word() == TWordPosition::FIRST_CHILD) // new sentence started
                    CheckStoreTextBreaks();
            }
        }
        break;

    case NLP_SENTBREAK:
    case NLP_PARABREAK:
        if (!IgnoreStoreTextBreaks) {
            OnSpaces(type, tok.Token, tok.Leng);
            if (NextBreak())
                CheckStoreTextBreaks();
        } else {
            OnSpaces(NLP_MISCTEXT, tok.Token, tok.Leng);
        }
        break;

    default:
        OnSpaces(type, tok.Token, tok.Leng);
        break;
    }
}

void TDirectTokenizer::CheckBreak() {
    if (!IgnoreStoreTextBreaks)
        NextBreak();
}

void TDirectTokenizer::DoStoreText(const wchar16* text, size_t len, RelevLevel relev, const IRelevLevelCallback* callback) {
    // we can't store text anymore, current sentence is full
    if (NoImplicitBreaks && CurrentPos.Word() == WORD_LEVEL_Max)
        return;
    StoreTextBreaks = 0;
    CurrentPos.SetRelevLevel(relev);
    RelevLevelCallback = callback;
    TNlpTokenizer Toker(*this, BackwardCompatible);
    Toker.Tokenize(text, len);
    RelevLevelCallback = nullptr;
    OnBreak();
}

void TDirectTokenizer::StoreText(const wchar16* text, size_t len, RelevLevel relev, const IRelevLevelCallback* callback) {
    CheckBreak();
    DoStoreText(text, len, relev, callback);
}

void TDirectTokenizer::OpenZone(const TString& zoneName) {
    Creator.StoreZone(DTZoneSearch|DTZoneText, zoneName.data(), CurrentPos.DocLength(), true);
}

void TDirectTokenizer::CloseZone(const TString& zoneName) {
    Creator.StoreZone(DTZoneSearch|DTZoneText, zoneName.data(), CurrentPos.DocLength(), false);
}

}
