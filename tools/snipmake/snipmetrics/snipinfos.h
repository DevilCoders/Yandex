#pragma once

#include "pixellength.h"
#include "readability.h"

#include <tools/snipmake/common/common.h>

#include <kernel/snippets/archive/view/storage.h>
#include <kernel/snippets/sent_info/sent_info.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <yweb/autoclassif/pornoclassifier/pornotermweight.h>

namespace NSnippets {

    struct TSnipExtInfo {
        THolder<TArchiveStorage> Storage;
        THolder<TSentsInfo> SentsInfo;
        THolder<TSentsMatchInfo> Match;
        int FirstSnipWordId;
        size_t SnipTextOffset;
        bool IsByLink = false;
        TSnipExtInfo(const TQueryy& query, const TReqSnip& reqSnip, const TConfig& cfg);
    };


    struct TSitelinkStatInfo {
        size_t WordsCount;
        size_t SymbolsCount;
        double ColoredWordsRate;
        double DigitsRate;
        double UppercaseLettersRate;
        bool IsUnknownLanguage;
        bool IsDifferentLanguage;
        bool HasAlpha;
        TSitelinkStatInfo(const TSitelink& sitelinks, const TQueryy& query, const TConfig& cfg);
    };

    struct TSnipStatInfo {
        int SnipLenInSents;
        int SnipLenInWords;
        int SnipLenInBytes;
        size_t Less4WordsFragmentsCount;
        double TitleWordsInSnipPercent;
        TSnipStatInfo(const TSnipExtInfo& snipExtInfo);
    };

    struct TColoringInfo {
        double TitleBoldedRate;
        double SnippetBoldedRate;
        int LinksCount;
        TColoringInfo(const TSnipExtInfo& snipExtInfo, const TReqSnip& reqSnip);
    };

    struct TQueryWordsInSnipInfo {
        int MaxChainLen;
        int MatchesCount;
        int QueryWordsCount;
        double NonstopQueryWordsCount;
        double QueryWordsFoundInTitlePercent;
        int FirstMatchPos;
        bool HasQueryWordsInTitle;
        int NonstopUserWordCount;
        int NonstopLikeUserLemmaCount;
        int NonstopCleanUserLemmaCount;
        int NonstopUserWizCount;
        int NonstopUserExtenCount;
        double NonstopUserWordRate;
        double NonstopUserLemmaRate;
        double NonstopUserExtenRate;
        double UniqueWordRate;
        double UniqueLemmaRate;
        double UniqueGroupRate;
        double NonstopUserWordDensity;
        double NonstopUserLemmaDensity;
        double NonstopUserCleanLemmaDensity;
        double NonstopUserExtenDensity;
        double NonstopUserWizDensity;
        int NonstopUserSynonymsCount;
        TQueryWordsInSnipInfo(const TSnipExtInfo& snipExtInfo);
    };

    struct TSnipReadabilityInfo {
        double NotReadableCharsRate;
        double DiffLanguageWordsRate;
        double UppercaseLettersRate;
        double DigitsRate;
        double TrashWordsRate;
        double AverageWordLength;
        int FragmentsCount;
        bool HasUrl;
        bool HasMenuLike;                     // More then 3 consecutive words, which start with uppercase letters.
        bool HasPornoWords;
        TSymbolsStat& CharsStat;
        TSnipReadabilityInfo(const TSnipExtInfo& snipExtInfo, const TPornoTermWeight& pornoWeighter, TSymbolsStat& Inspector);
    };


    class TPixelLengthInfo {
    private:
        PixelLenResult YandexLenResult;
        PixelLenResult GoogleLenResult;
    public:
        TPixelLengthInfo(const TSnipExtInfo& extSnipInfo)
            : YandexLenResult(GetYandexPixelLen(*extSnipInfo.Match, extSnipInfo.SnipTextOffset))
            , GoogleLenResult(GetGooglePixelLen(*extSnipInfo.Match, extSnipInfo.SnipTextOffset))
        {
        }

        int GetYandexStringNum() const {
            return YandexLenResult.StringNum;
        }

        float GetYandexLastStringFill() const {
           return YandexLenResult.LastStringFill;
        }

        int GetGoogleStringNum() const {
            return GoogleLenResult.StringNum;
        }

        float GetGoogleLastStringFill() const {
            return GoogleLenResult.LastStringFill;
        }
    };
}
