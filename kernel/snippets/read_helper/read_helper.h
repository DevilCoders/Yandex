#pragma once

#include <library/cpp/langs/langs.h>

#include <util/generic/string.h>

namespace NSnippets {
    class TSentsMatchInfo;
    class TConfig;
    class TQueryy;
    class TSnip;

    class TReadabilityHelper {
    private:
        bool TurkishThresholds = false;
    public:
        int Alpha = 0;
        int Digit = 0;
        int Slash = 0;
        int Vert = 0;
        int Punct = 0;
        int PunctBal = 0;
        int TrashAscii = 0;
        int TrashUTF = 0;
        int Words = 0;
        int SymLen = 0;
        int Phones = 0;
        int Dates = 0;
    public:
        TReadabilityHelper(bool turkishThresholds);
        void AddRange(const TSentsMatchInfo& info, int i, int j);
        void AddSnip(const TSnip& snip);
        double CalcReadability() const;
    };

    class TReadabilityChecker {
    private:
        const TConfig& Cfg;
        const TQueryy& Query;
        const ELanguage LangId;
    public:
        bool CheckShortSentences = false;
        bool CheckBadCharacters = false;
        bool CheckLanguageMatch = false;
        bool CheckCapitalization = false;
        bool CheckRepeatedWords = false;
    public:
        TReadabilityChecker(const TConfig& cfg, const TQueryy& query, ELanguage langId = LANG_UNK);
        bool IsReadable(const TUtf16String& text, bool isAlgoSnip);
    };

    float CalcReadability(const TUtf16String& text);
}
