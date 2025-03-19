#include "read_helper.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/qtree/query.h>
#include <kernel/snippets/sent_info/sent_info.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/retained_info.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/wordstat/wordstat_data.h>

#include <util/charset/unidata.h>
#include <util/generic/utility.h>

namespace NSnippets {
    TReadabilityHelper::TReadabilityHelper(bool turkishThresholds)
        : TurkishThresholds(turkishThresholds)
    {
    }

    void TReadabilityHelper::AddRange(const TSentsMatchInfo& info, int i, int j) {
        Phones += info.TelephonesInRange(i, j);
        Dates += info.DatesInRange(i, j);
        Words += j - i + 1;
        Alpha += info.AlphasInRange(i, j);
        Digit += info.DigitsInrange(i, j);
        Slash += info.SlashesInGapsInRange(i, j);
        Vert += info.VertsInGapsInRange(i, j);
        Punct += info.PunctReadInGapInRange(i, j);
        PunctBal += info.PunctBalInGapInRange(i, j);
        TrashAscii += info.TrashAsciiInGapInRange(i, j);
        TrashUTF += info.TrashUTFInGapInRange(i, j);
        SymLen += info.SentsInfo.GetWordSentSpanBuf(i, j).size();
    }

    double TReadabilityHelper::CalcReadability() const {
        if (SymLen <= 0) {
            return 1.0;
        }

        int punctBal = PunctBal;
        if (punctBal < 0) {
            punctBal = -punctBal;
        }
        int digit = Digit;
        int punct = Punct;


        // a piece of hack
        if (Phones > 0) {
            digit = Max(0, digit - 7);
            punct = Max(0, punct - 2);
        }
        if (Dates > 0) {
            digit = Max(0, digit - 6);
            punct = Max(0, punct - 2);
        }

        double shareOfAlpha = static_cast<double>(Alpha) / SymLen;
        if (shareOfAlpha < 0.618556701) {
            shareOfAlpha = 1. - shareOfAlpha;
            return shareOfAlpha;
        }
        shareOfAlpha = 1. - shareOfAlpha;

        // Use different readability params in Turkey
        double asciiThreshold = TurkishThresholds ? 0.04 : 0.03;
        if (static_cast<double>(TrashAscii) / SymLen > asciiThreshold) {
            return shareOfAlpha;
        }

        if (static_cast<double>(TrashUTF) / SymLen > 0.015) {
            return shareOfAlpha;
        }

        int slashThreshold = TurkishThresholds ? 3 : 2;
        if (Slash >= slashThreshold) {
            return shareOfAlpha;
        }

        if (Vert > 1) {
            return shareOfAlpha;
        }

        if (punctBal > 2) {
            return shareOfAlpha;
        }

        double digitThreshold = TurkishThresholds ? 0.145 : 0.16;
        if (static_cast<double>(digit) / SymLen > digitThreshold) {
            return shareOfAlpha;
        }

        double punctThreshold = TurkishThresholds ? 0.85 : 0.9;
        if (Words > 0 && static_cast<double>(punct) / Words > punctThreshold) {
            return shareOfAlpha;
        }

        return 0;
    }

    void TReadabilityHelper::AddSnip(const TSnip& snip) {
        for (const TSingleSnip& ssnip : snip.Snips) {
            AddRange(*ssnip.GetSentsMatchInfo(), ssnip.GetFirstWord(), ssnip.GetLastWord());
        }
    }

    TReadabilityChecker::TReadabilityChecker(const TConfig& cfg, const TQueryy& query, ELanguage langId)
        : Cfg(cfg)
        , Query(query)
        , LangId(langId)
    {
    }

    bool TReadabilityChecker::IsReadable(const TUtf16String& text, bool isAlgoSnip) {
        TRetainedSentsMatchInfo customSents;
        customSents.SetView(text, TRetainedSentsMatchInfo::TParams(Cfg, Query).SetLang(LangId));
        const TSentsMatchInfo& smi = *customSents.GetSentsMatchInfo();
        int wordCount = smi.WordsCount();
        int sentencesCount = smi.SentsInfo.SentencesCount();
        if (wordCount == 0) {
            return false;
        }

        if (CheckShortSentences) {
            if (sentencesCount * 2 > wordCount) {
                return false;
            }
        }

        if (CheckBadCharacters) {
            TReadabilityHelper readHelper(Cfg.UseTurkishReadabilityThresholds());
            readHelper.AddRange(smi, 0, wordCount - 1);
            if (readHelper.CalcReadability() >= (isAlgoSnip ? 0.5 : 1e-5)) {
                return false;
            }
        }

        if (CheckLanguageMatch && !isAlgoSnip) {
            int langMatch = smi.LangMatchsInRange(0, wordCount - 1);
            if (langMatch * 2 < wordCount) {
                return false;
            }
        }

        if (CheckRepeatedWords) {
            TWordStatData wordStatData(Query, wordCount);
            wordStatData.PutSpan(smi, 0, wordCount - 1);
            if (wordStatData.RepeatedWordsScore > 0) {
                return false;
            }
        }

        if (CheckCapitalization) {
            size_t caps = 0;
            for (wchar16 c : text) {
                if (ToUpper(c) == c && ToLower(c) != c) {
                    ++caps;
                }
            }
            if (caps * 3 > text.size()) {
                return false;
            }
        }

        return true;
    }

    float CalcReadability(const TUtf16String& text) {
        TConfig cfg;
        TQueryy queryy(nullptr, cfg);
        TRetainedSentsMatchInfo customSents;
        customSents.SetView(text, TRetainedSentsMatchInfo::TParams(cfg, queryy));
        const TSentsMatchInfo& smi = *customSents.GetSentsMatchInfo();
        int wordCount = smi.WordsCount();
        if (wordCount == 0) {
            return 0.f;
        }
        TReadabilityHelper readHelper(false);
        readHelper.AddRange(smi, 0, wordCount - 1);
        return readHelper.CalcReadability();
    }
}
