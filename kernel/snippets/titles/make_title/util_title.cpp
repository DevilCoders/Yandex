#include "util_title.h"

#include "make_title.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/qtree/query.h>
#include <kernel/snippets/sent_info/sent_info.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/retained_info.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/sent_match/similarity.h>

#include <library/cpp/lcs/lcs_via_lis.h>

#include <util/charset/unidata.h>
#include <util/charset/wide.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/ascii.h>

namespace NSnippets {
    struct TSnipTitle::TImpl {
        TUtf16String TitleString;
        TRetainedSentsMatchInfo Source;
        TSnip TitleSnip;
        TTitleFactors TitleFactors;
        TDefinition Definition;
        TSimpleSharedPtr<TEQInfo> EQInfo;
    };
    TSnipTitle::TSnipTitle()
        : Impl(new TImpl)
    {
    }
    TSnipTitle::TSnipTitle(const TUtf16String& titleString, const TRetainedSentsMatchInfo& source, const TSnip& titleSnip, const TTitleFactors& titleFactors, const TDefinition& definition)
        : Impl(new TImpl)
    {
        Impl->TitleString = titleString;
        Impl->Source = source;
        Impl->TitleSnip = titleSnip;
        Impl->TitleFactors = titleFactors;
        Impl->Definition = definition;
    }
    TSnipTitle::TSnipTitle(const TUtf16String& titleString, const TConfig& cfg, const TQueryy& query)
        : Impl(new TImpl)
    {
        Impl->TitleString = titleString;
        Impl->Source.SetView(titleString, TRetainedSentsMatchInfo::TParams(cfg, query));
        const TSentsMatchInfo& smInfo = *Impl->Source.GetSentsMatchInfo();
        if (smInfo.WordsCount() > 0) {
            Impl->TitleSnip.Snips.push_back(TSingleSnip(0, smInfo.WordsCount() - 1, smInfo));
        }
    }
    TSnipTitle::TSnipTitle(const TSnipTitle& other)
        : Impl(new TImpl(*other.Impl.Get()))
    {
    }
    TSnipTitle::~TSnipTitle() {
    }
    TSnipTitle& TSnipTitle::operator=(const TSnipTitle& other) {
        *Impl.Get() = *other.Impl.Get();
        return *this;
    }
    const TSentsInfo* TSnipTitle::GetSentsInfo() const {
        return Impl->Source.GetSentsInfo();
    }
    const TSentsMatchInfo* TSnipTitle::GetSentsMatchInfo() const {
        return Impl->Source.GetSentsMatchInfo();
    }
    const TUtf16String& TSnipTitle::GetTitleString() const {
        return Impl->TitleString;
    }
    float TSnipTitle::GetPixelLength() const {
        return Impl->TitleFactors.PixelLength;
    }
    const TSnip& TSnipTitle::GetTitleSnip() const {
        return Impl->TitleSnip;
    }
    double TSnipTitle::GetPLMScore() const {
        return Impl->TitleFactors.PLMScore;
    }
    double TSnipTitle::GetLogMatchIdfSum() const {
        return Impl->TitleFactors.LogMatchIdfSum;
    }
    double TSnipTitle::GetMatchUserIdfSum() const {
        return Impl->TitleFactors.MatchUserIdfSum;
    }
    int TSnipTitle::GetSynonymsCount() const {
        return Impl->TitleFactors.SynonymsCount;
    }
    double TSnipTitle::GetQueryWordsRatio() const {
        return Impl->TitleFactors.QueryWordsRatio;
    }
    bool TSnipTitle::HasRegionMatch() const {
        return Impl->TitleFactors.HasRegionMatch;
    }
    bool TSnipTitle::HasBreak() const {
        return Impl->TitleFactors.HasBreak;
    }
    const TEQInfo& TSnipTitle::GetEQInfo() const {
        if (!Impl->EQInfo) {
            Impl->EQInfo = new TEQInfo(Impl->TitleSnip);
        }
        return *Impl->EQInfo;
    }
    const TDefinition& TSnipTitle::GetDefinition() const {
        return Impl->Definition;
    }

    static const TUtf16String TURKISH_DIACRITICS = u"çğıöşü";
    static const TString REMOVED_TURKISH_DIACRITICS = "cgiosu";

    TString GetLatinLettersAndDigits(const TWtringBuf& str) {
        TString goodSymbols;
        for (wchar16 c : str) {
            wchar16 symbol = ToLower(c);
            size_t diacriticsPosition = TURKISH_DIACRITICS.find(symbol);
            if (diacriticsPosition != TUtf16String::npos) {
                symbol = REMOVED_TURKISH_DIACRITICS[diacriticsPosition];
            }
            if (symbol == '.' || IsAsciiAlpha(symbol) || IsDigit(symbol)) {
                goodSymbols.push_back(symbol);
            }
        }
        return goodSymbols;
    }

    bool SimilarTitleStrings(const TUtf16String& string1, const TUtf16String& string2, double ratio, bool ignoreDiacriticsForTurkey) {
        TUtf16String clearedString1, clearedString2;
        for (size_t i = 0; i < string1.size(); ++i)
            if ((!ignoreDiacriticsForTurkey || (size_t)string1[i] <= 0x7F) && IsAlpha(string1[i]))
                clearedString1.push_back(ToLower(string1[i]));
        if (clearedString1.empty())
            return true;
        for (size_t i = 0; i < string2.size(); ++i)
            if ((!ignoreDiacriticsForTurkey || (size_t)string2[i] <= 0x7F) && IsAlpha(string2[i]))
                clearedString2.push_back(ToLower(string2[i]));
        if (clearedString2.empty())
            return true;
        size_t n = clearedString1.size();
        size_t m = clearedString2.size();
        return NLCS::MeasureLCS<size_t>(clearedString1, clearedString2) > ratio * Min(n, m);
    }

    void EraseFullStop(TUtf16String& line) {
        if (!line.empty() && line.back() == '.') {
            line.pop_back();
            if (!line.empty() && line.back() == '.') {
                line.push_back('.');
            }
        }
    }

    static const TUtf16String PIPE_SEPARATOR = u" | ";
    static const TUtf16String EM_DASH_SEPARATOR = u" — ";
    static const TUtf16String EN_DASH_SEPARATOR = u" – ";

    const TUtf16String& GetTitleSeparator(const TConfig& cfg) {
        if (cfg.ExpFlagOn("em_dash_title_separator")) {
            return EM_DASH_SEPARATOR;
        }
        if (cfg.ExpFlagOn("en_dash_title_separator")) {
            return EN_DASH_SEPARATOR;
        }
        return PIPE_SEPARATOR;
    }

    static constexpr float IDF_LOW_THRESHOLD = 0.5f;
    static constexpr float IDF_RATIO_LOW_THRESHOLD = 0.75f;
    static constexpr float QUERY_WORDS_RATIO_THRESHOLD = 0.25f;
    static constexpr float MIN_TITLES_SIMILARITY_RATIO = 0.8f;
    static constexpr float MIN_TITLES_LEN_RATIO = 1.3f;

    bool IsCandidateBetterThanTitle(const TQueryy& query, const TSnipTitle& candidateInfo, const TSnipTitle& titleInfo, bool altheaders3, bool glued) {
        if (glued) {
            if (candidateInfo.GetSynonymsCount() > titleInfo.GetSynonymsCount()) {
                return true;
            }
            if (candidateInfo.GetQueryWordsRatio() > 1E-7 && titleInfo.GetQueryWordsRatio() < 1E-7) {
                return true;
            }
        } else {
            if (altheaders3 && titleInfo.HasBreak() && !candidateInfo.HasBreak() &&
                candidateInfo.GetSynonymsCount() >= titleInfo.GetSynonymsCount() &&
                candidateInfo.GetMatchUserIdfSum() + 1E-7 > titleInfo.GetMatchUserIdfSum() &&
                candidateInfo.GetTitleString().size() * MIN_TITLES_LEN_RATIO > titleInfo.GetTitleString().size()  &&
                SimilarTitleStrings(titleInfo.GetTitleString(), candidateInfo.GetTitleString(), MIN_TITLES_SIMILARITY_RATIO))
            {
                return true;
            }
        }

        if (candidateInfo.GetQueryWordsRatio() <= QUERY_WORDS_RATIO_THRESHOLD) {
            return false;
        }
        if (titleInfo.GetTitleSnip().WordsCount() >= 3 && titleInfo.GetQueryWordsRatio() > 1 - 1E-7) {
            return false;
        }

        bool candidateContainsAllSynonyms = candidateInfo.GetSynonymsCount() == query.NonstopUserPosCount();
        bool candidateIsReallyBold = candidateInfo.GetTitleSnip().WordsCount() >= 4 && candidateInfo.GetQueryWordsRatio() > 1 - 1E-7;
        bool titleIsReallyPale = titleInfo.GetQueryWordsRatio() < 1E-7;
        if ((candidateContainsAllSynonyms || candidateIsReallyBold) && titleIsReallyPale) {
            return true;
        }
        return candidateInfo.GetMatchUserIdfSum() > IDF_LOW_THRESHOLD &&
               candidateInfo.GetMatchUserIdfSum() * IDF_RATIO_LOW_THRESHOLD > titleInfo.GetMatchUserIdfSum() &&
               candidateInfo.GetSynonymsCount() > 1 &&
               candidateInfo.GetSynonymsCount() > titleInfo.GetSynonymsCount();
    }

}
