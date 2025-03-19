#pragma once

#include "factors.h"

#include <kernel/mango/common/types.h>

namespace NMango
{
namespace NDetail
{

template<class T1, class T2>
inline static void Copy(const T1 &left, T2 &right)
{
    right = left;
}

template<class T1, class T2>
inline static void Copy(T1 &left, const T2 &right)
{
    left = static_cast<T1>(Min<T2>(Max<T1>(), right));
}

template<typename T1, typename T2, typename T3>
void DoMappingErf(T1 erf, T2 accStat, T3 accOth)
{
    Copy(erf.LinksCount, accStat[LINKS_COUNT]);
    Copy(erf.MaxAuthority, accStat[MAX_AUTHORITY]);
    Copy(erf.AuthorCount, accStat[AUTHOR_COUNT]);
    Copy(erf.Diversity, accStat[DIVERSITY]);
    Copy(erf.AuthorAuthoritySum, accStat[AUTHOR_AUTHORITY_SUM]);
    Copy(erf.QuoteAuthoritySum, accStat[QUOTE_AUTHORITY_SUM]);
    Copy(erf.AuthorAuthorityQ50, accStat[AUTHOR_AUTHORITY_Q50]);
    Copy(erf.AuthorAuthorityQ75, accStat[AUTHOR_AUTHORITY_Q75]);
    Copy(erf.AuthorAuthorityQ90, accStat[AUTHOR_AUTHORITY_Q90]);
    Copy(erf.AuthorAuthorityAvg75, accStat[AUTHOR_AUTHORITY_AVG75]);
    Copy(erf.SpamCount, accStat[SPAM_COUNT]);

    Copy(erf.CreationTime, accOth[CREATION_TIME]);
    Copy(erf.TweetCount, accOth[TWEET_COUNT]);
    Copy(erf.NewsAuthorityBoost, accOth[AUTH_PREDICT_KOEF]);
    Copy(erf.LikeCount, accOth[LIKE_COUNT]);
    Copy(erf.LikeAuthoritySum, accOth[LIKE_AUTHORITY_SUM]);
    Copy(erf.AuthorityStatrank, accOth[AUTHORITY_STATRANK]);
}

template<typename T1, typename T2>
void DoMappingHerf(T1 erf, T2 acc)
{
    Copy(erf.HostLinksCount, acc[LINKS_COUNT]);
    Copy(erf.HostResourceCount, acc[RESOURCE_COUNT]);
    Copy(erf.HostMaxAuthority, acc[MAX_AUTHORITY]);
    Copy(erf.HostAuthorCount, acc[AUTHOR_COUNT]);
    Copy(erf.HostAuthorAuthoritySum, acc[AUTHOR_AUTHORITY_SUM]);
    Copy(erf.HostQuoteAuthoritySum, acc[QUOTE_AUTHORITY_SUM]);
    Copy(erf.HostAuthorAuthorityQ50, acc[AUTHOR_AUTHORITY_Q50]);
    Copy(erf.HostAuthorAuthorityQ75, acc[AUTHOR_AUTHORITY_Q75]);
    Copy(erf.HostAuthorAuthorityQ90, acc[AUTHOR_AUTHORITY_Q90]);
    Copy(erf.HostAuthorAuthorityAvg75, acc[AUTHOR_AUTHORITY_AVG75]);
    Copy(erf.HostSpamCount, acc[SPAM_COUNT]);
}

template<typename T1, typename T2>
void DoMappingErfHerf(T1 erf, T2 herf)
{
    Copy(erf.HostLinksCount, herf.LinksCount);
    Copy(erf.HostResourceCount, herf.ResourceCount);
    Copy(erf.HostMaxAuthority, herf.MaxAuthority);
    Copy(erf.HostAuthorCount, herf.AuthorCount);
    Copy(erf.HostAuthorAuthoritySum, herf.AuthorAuthoritySum);
    Copy(erf.HostQuoteAuthoritySum, herf.QuoteAuthoritySum);
    Copy(erf.HostAuthorAuthorityQ50, herf.AuthorAuthorityQ50);
    Copy(erf.HostAuthorAuthorityQ75, herf.AuthorAuthorityQ75);
    Copy(erf.HostAuthorAuthorityQ90, herf.AuthorAuthorityQ90);
    Copy(erf.HostAuthorAuthorityAvg75, herf.AuthorAuthorityAvg75);
    Copy(erf.HostSpamCount, herf.SpamCount);
}

} // NDetail

struct TErfMapper
{
    static void FactorsToErf(const float* factors, TErfInfo& erf)
    {
        TConstAccessor<GROUP_QUOTE_STATIC> staticAcc(factors);
        TConstAccessor<GROUP_OTHER_STATIC> othAcc(factors);
        NDetail::DoMappingErf< TErfInfo&, TConstAccessor<GROUP_QUOTE_STATIC>&, TConstAccessor<GROUP_OTHER_STATIC>& >(erf, staticAcc, othAcc);

        erf.LangsCount        = static_cast<ui8>(Min<float>(Max<ui8>(), staticAcc[LANGS_COUNT]));
        erf.IsMorda           = othAcc[IS_MORDA];

        TConstAccessor<GROUP_QUOTE_HOST> hostAcc(factors);
        NDetail::DoMappingHerf< TErfInfo&, TConstAccessor<GROUP_QUOTE_HOST>& >(erf, hostAcc);
    }

    static void ErfToFactors(const TErfInfo &erf, float* factors)
    {
        TAccessor<GROUP_QUOTE_STATIC> staticAcc(factors);
        TAccessor<GROUP_OTHER_STATIC> othAcc(factors);
        NDetail::DoMappingErf< const TErfInfo&, TAccessor<GROUP_QUOTE_STATIC>&, TAccessor<GROUP_OTHER_STATIC>& >(erf, staticAcc, othAcc);

        staticAcc[LANGS_COUNT]       = erf.LangsCount;
        othAcc[IS_MORDA]             = erf.IsMorda;

        TAccessor<GROUP_QUOTE_HOST> hostAcc(factors);
        NDetail::DoMappingHerf< const TErfInfo&, TAccessor<GROUP_QUOTE_HOST>& >(erf, hostAcc);
    }

    static void FillDummyHostFields(TErfInfo &erf)
    {
        erf.HostLinksCount = erf.LinksCount;
        erf.HostResourceCount = 1;
        erf.HostMaxAuthority = erf.MaxAuthority;
        erf.HostAuthorCount = erf.AuthorCount;
        erf.HostAuthorAuthoritySum = erf.AuthorAuthoritySum;
        erf.HostQuoteAuthoritySum = erf.QuoteAuthoritySum;
        erf.HostAuthorAuthorityQ50 = erf.AuthorAuthorityQ50;
        erf.HostAuthorAuthorityQ75 = erf.AuthorAuthorityQ75;
        erf.HostAuthorAuthorityQ90 = erf.AuthorAuthorityQ90;
        erf.HostAuthorAuthorityAvg75 = erf.AuthorAuthorityAvg75;
        erf.HostSpamCount = erf.SpamCount;
    }

    static void FillHostFields(TErfInfo &erf, const THerfInfo& herf)
    {
        NDetail::DoMappingErfHerf< TErfInfo&, const THerfInfo& >(erf, herf);
    }

    static void ExtractHerf(const TErfInfo &erf, THerfInfo& herf)
    {
        NDetail::DoMappingErfHerf< const TErfInfo&, THerfInfo& >(erf, herf);
    }
};

} // NMango
