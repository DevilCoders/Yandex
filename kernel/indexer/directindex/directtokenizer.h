#pragma once

#include <library/cpp/langmask/langmask.h>
#include <library/cpp/token/nlptypes.h>
#include <library/cpp/langmask/index_language_chooser.h>
#include <library/cpp/tokenizer/tokenizer.h>
#include <library/cpp/wordpos/wordpos.h>

#include <library/cpp/langs/langs.h>
#include <util/generic/string.h>

namespace NIndexerCore {

class TDirectTextCreator;

class IRelevLevelCallback {
protected:
    virtual ~IRelevLevelCallback() {}
public:
    virtual RelevLevel OnToken(NLP_TYPE type, const TWideToken& token) const = 0;
};

class TDirectTokenizer : public ITokenHandler {
protected:
    TDirectTextCreator& Creator;
private:
    const bool BackwardCompatible;
    bool PosOK;               // PosOK устанавливается в false, когда происходит переполнение в CurrentPos
    TWordPosition CurrentPos; // Текущая позиция в документе
    ui32 WordCount;

    TIndexLanguageOptions OldLanguageOptions;
    ui8 LemmerOptions;

    const IRelevLevelCallback* RelevLevelCallback;
public:
    bool IgnoreStoreTextBreaks;   // Игнорирование конца предложения от StoreText
    bool IgnoreStoreTextNextWord; // Игнорирование любого увеличения словопозиции кроме прямого вызова NextWord()
    bool NoImplicitBreaks;
    ui32 StoreTextBreaks;         // counter of breaks on single StoreText() call
    ui32 StoreTextMaxBreaks;      // maximum number of breaks on single StoreText() call
    bool IgnoreSpecialKeys = false; // ignore one-symbol tokens like unicode emoji, math symbols; see SEARCH-3783
public:
    TDirectTokenizer(TDirectTextCreator& creator, bool backwardCompatible = false, const TWordPosition& startPos = TWordPosition(0, 1, 1));
    ui32 GetWordCount() const {
        return WordCount;
    }
    TWordPosition GetPosition() const {
        return CurrentPos;
    }
    void SetLangOptions(const TLangMask& langMask, ELanguage lang, ui8 lemmerOptions);
    void ResetLangOptions();
    void NextWord() {
        if (NoImplicitBreaks && CurrentPos.Word() == WORD_LEVEL_Max)
            ythrow TAllDoneException();
        PosOK = CurrentPos.Inc();
        WordCount++;
    }
    void IncBreak(ui32 k);

    void AddDoc(ui32 docId, ELanguage lang);
    void CommitDoc();

    void StoreText(const wchar16* text, size_t len, RelevLevel relev, const IRelevLevelCallback* callback = nullptr);
    void OpenZone(const TString& zoneName);
    void CloseZone(const TString& zoneName);

    void OnToken(const TWideToken& tok, size_t origleng, NLP_TYPE type) override;
protected:
    void CheckBreak();
    void DoStoreText(const wchar16* text, size_t len, RelevLevel relev, const IRelevLevelCallback* callback);
private:
    // returns true if sentence number was increased
    bool NextBreak();
    void OnTokenStart(const TWideToken& tok);
    void OnSpaces(NLP_TYPE type, const wchar16* token, size_t len);
    virtual void OnBreak() {
    }
    // it is called from OnToken() only
    void CheckStoreTextBreaks() {
        Y_ASSERT(StoreTextBreaks < StoreTextMaxBreaks);
        if (++StoreTextBreaks == StoreTextMaxBreaks)
            ythrow TAllDoneException();
    }
};

}
