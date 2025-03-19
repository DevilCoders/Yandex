#include "hits_based_tr.h"
#include "quorum.h"

#include <kernel/mango/common/constraints.h>

#include <util/generic/algorithm.h>
#include <util/generic/utility.h>
#include <util/generic/bitmap.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>

namespace NMango
{

#define EQUAL_NO_MATCH  NUM_FORM_CLASSES
#define NoValue         static_cast<ui32>(-1)

inline float GetFormClassWeight(EFormClass formClass)
{
    static_assert(EQUAL_BY_STRING == 0, "expect EQUAL_BY_STRING == 0");
    static_assert(EQUAL_BY_LEMMA == 1, "expect EQUAL_BY_LEMMA == 1");
    static_assert(EQUAL_BY_SYNONYM == 2, "expect EQUAL_BY_SYNONYM == 2");
    static_assert(EQUAL_BY_SYNSET == 3, "expect EQUAL_BY_SYNSET == 3");

    if (formClass >= 4 || formClass < 0) {
        return 0.0f;
    }

#ifdef MANGO_TURKEY
    const static float weights[] = {1.0f, 0.1f, 0.03f, 0.03};
#else
    const static float weights[] = {1.0f, 0.35f, 0.3f, 0.1f};
#endif

    return weights[formClass];
}

inline float GetQuorumClassWeight(EFormClass formClass)
{
    static_assert(EQUAL_BY_STRING == 0, "expect EQUAL_BY_STRING == 0");
    static_assert(EQUAL_BY_LEMMA == 1, "expect EQUAL_BY_LEMMA == 1");
    static_assert(EQUAL_BY_SYNONYM == 2, "expect EQUAL_BY_SYNONYM == 2");
    static_assert(EQUAL_BY_SYNSET == 3, "expect EQUAL_BY_SYNSET == 3");

    if (formClass >= 4 || formClass < 0) {
        return 0.0f;
    }

    const static float weights[] = {1.0f, 1.0f, 0.5f, 0.5f};
    return weights[formClass];
}

inline float GetRelevLevelWeight(RelevLevel relevLevel)
{
    static_assert(LOW_RELEV == 0, "expect LOW_RELEV == 0");
    static_assert(MID_RELEV == 1, "expect MID_RELEV == 1");
    static_assert(HIGH_RELEV == 2, "expect HIGH_RELEV == 2");
    static_assert(BEST_RELEV == 3, "expect BEST_RELEV == 3");

    if (relevLevel >= 4 || relevLevel < 0) {
        return 0.0f;
    }

    const static float weights[] = {0.4f, 0.67f, 0.84f, 1.0f};
    return weights[relevLevel];
}

inline float GetStopWordMultiplier(bool isStopWord)
{
    if (isStopWord) {
        return 0.7f;
    } else {
        return 1.0f;
    }
}

#define MAX_DIST 1024

class THitCounter
{
public:
    TVector<EFormClass> QuorumHitTypes;
    TVector<float>  HardRelevWeights;
    TVector<float>  QuorumRfs;
    TVector<size_t> QueryWordGlobalPositions;
    TVector<size_t> QueryWordInQuotePositions;
    float        QueryNorm, Scalar;
    ui32         NumHits;
    THitInfo     OldHit;

    static const float DEFAULT_KPM_COEF_VALUE;// according to c++03 standart not here = 1.0;

    float        KeyPhraseMatchCoef;
    const TLinkInfo* LinkInfo;

    float        LastWordWeightSum;
    bool         FastQuorumMode;
    int          HitStatistics[NUM_RELEVS];
    float        BestMangoLr;
    float        BestMangoQuorum;
    int          WordCountNonStop;
    bool         IsPerfectMatch;
    const TQueryWordInfos* Infos;
    const TProximityInfos* Proxes;
public:
    void Reset(const TQueryWordInfos& infos, bool fastQuorumMode, const TProximityInfos* proxes, const TLinkInfo* linkInfo)
    {
        size_t size = infos.size();
        Infos = &infos;
        Proxes = proxes;
        LinkInfo = linkInfo;
        QuorumHitTypes.assign(size, EQUAL_NO_MATCH);
        QuorumRfs.assign(size, 0.0f);
        HardRelevWeights.assign(size, 1.0f);
        float stopWeight = GetStopWordMultiplier(true);
        stopWeight *= stopWeight;
        WordCountNonStop = 0;
        for (size_t i = 0; i < size; ++i) {
            if (infos[i].IsStopWord) {
                HardRelevWeights[i] = stopWeight;
            } else {
                ++WordCountNonStop;
            }
            QuorumRfs[i] = pow(infos[i].Weight, 0.25f);
        }
        WordCountNonStop = Max(WordCountNonStop, 1);
        FastQuorumMode = fastQuorumMode;
        BestMangoLr = BestMangoQuorum = 0.0f;
        IsPerfectMatch = false;

        if (!FastQuorumMode) {
            QueryWordGlobalPositions.assign(size, NoValue);
            QueryWordInQuotePositions.assign(size, NoValue);
            KeyPhraseMatchCoef = DEFAULT_KPM_COEF_VALUE;
            Scalar = 0.0f;
            QueryNorm = MATCH_EPS;
            NumHits = 0;
            OldHit.GlobalWordPosition = 0x7fffffff;
            OldHit.QueryWordIndex = 0;
            LastWordWeightSum = 0.0f;
            Zero(HitStatistics);
        }
    }

    inline int CalcDistValue(int pos, int oldpos)
    {
        if (pos >= oldpos) {
            return pos - oldpos;
        } else {
            return 1 + oldpos - pos;
        }
    }

    inline float CombineTwoDistances(float dist1, float dist2)
    {
        if (dist1 < dist2) {
            std::swap(dist1, dist2);
        }
        if (dist2 == 0.0f) {
            return dist1;
        }
        return (dist1 * 3.0f + dist2) / 4.0f;
    }

    void OnHit(const THitInfo& hit, const TQueryWordInfo& qwi)
    {
        if (FastQuorumMode) {
            QuorumHitTypes[hit.QueryWordIndex] = Min(QuorumHitTypes[hit.QueryWordIndex], hit.Form);
            return;
        }

        if (hit.GlobalWordPosition != OldHit.GlobalWordPosition) {
            // норма запроса
            if (hit.Form == EQUAL_BY_STRING || hit.Form == EQUAL_BY_LEMMA) {
                QueryNorm += LastWordWeightSum * LastWordWeightSum;
                LastWordWeightSum = 0.0f;
            }

            OldHit.GlobalWordPosition = hit.GlobalWordPosition;
        }
        OldHit.QueryWordIndex = hit.QueryWordIndex;

        // числитель скалярного произведения
        // если это низкорелевантное слово или стоп-слово - то цитата в этом не виновата, такой уж запрос, принижаем и знаменатель
        // если это совпадение по неточной форме - это всецело вина цитаты, принижаем только числитель
        float relevMult = GetRelevLevelWeight(hit.Relev) * GetStopWordMultiplier(qwi.IsStopWord);
        float weight = qwi.Weight * relevMult;
        float formClassWeight = GetFormClassWeight(hit.Form);
        Scalar += weight * weight * formClassWeight;
        ++NumHits;

        // раньше не встречали такое слово запроса - учесть в норме запроса
        if (QuorumHitTypes[hit.QueryWordIndex] == EQUAL_NO_MATCH) {
            LastWordWeightSum += weight;
            HardRelevWeights[hit.QueryWordIndex] = relevMult * relevMult;
        } else {
            HardRelevWeights[hit.QueryWordIndex] = Max(HardRelevWeights[hit.QueryWordIndex], relevMult * relevMult);
        }

        // запомнить для истории
        QuorumHitTypes[hit.QueryWordIndex] = Min(QuorumHitTypes[hit.QueryWordIndex], hit.Form);
        if (!qwi.IsStopWord) {
            ++HitStatistics[hit.Relev];
        }

        // check proximities
        QueryWordGlobalPositions[hit.QueryWordIndex] = hit.GlobalWordPosition;
        if (Proxes) {
            for (size_t i = 0; i + 1 < QueryWordGlobalPositions.size(); ++i) {
                size_t pos1 = QueryWordGlobalPositions[i];
                size_t pos2 = QueryWordGlobalPositions[i + 1];
                int delta = static_cast<int>(pos2) - static_cast<int>(pos1);
                if (pos1 != NoValue && pos2 != NoValue && (delta < (*Proxes)[i].Beg || delta > (*Proxes)[i].End)) {
                    return;
                }
            }
        }

        // Mango relevance
        size_t oldPos = NoValue;
        float curMangoLr(0.0f), curMangoQuorum(0.0f), relSum(0.0f), quorumSum(0.0f), oldRelValue(0.0f), oldQuorumValue(0.0f);
        float oldQuorumDistWeight(0.0f), oldRelDistWeight(0.0f);
        bool isPerfectMatch = true;
        for (size_t i = 0; i < QueryWordGlobalPositions.size(); ++i) {
            float curRelValue = HardRelevWeights[i] * (*Infos)[i].Weight;
            float curQuorumValue = HardRelevWeights[i] * QuorumRfs[i];
            relSum += curRelValue;
            quorumSum += curQuorumValue;
            if (QueryWordGlobalPositions[i] != NoValue) {
                curRelValue *= GetFormClassWeight(QuorumHitTypes[i]);
                curQuorumValue *= GetQuorumClassWeight(QuorumHitTypes[i]);
                if (oldPos != NoValue) {
                    float dist = 1e-3f + Min(Max(0.0f, -1.0f + CalcDistValue(QueryWordGlobalPositions[i], oldPos)), 16.0f);
                    float curRelDistWeight = (1.0f - exp(-2.0f / dist));
                    float curQuorumDistWeight = (1.0f - exp(-7.0f / dist));

                    curMangoLr += oldRelValue * CombineTwoDistances(oldRelDistWeight, curRelDistWeight);
                    curMangoQuorum += oldQuorumValue * CombineTwoDistances(oldQuorumDistWeight, curQuorumDistWeight);

                    oldRelDistWeight = curRelDistWeight;
                    oldQuorumDistWeight = curQuorumDistWeight;
                }
                oldPos = QueryWordGlobalPositions[i];
                oldRelValue = curRelValue;
                oldQuorumValue = curQuorumValue;
            }
            isPerfectMatch = isPerfectMatch && (QuorumHitTypes[i] == EQUAL_BY_STRING)
                                            && (i == 0 || QueryWordGlobalPositions[i] == QueryWordGlobalPositions[i - 1] + 1);
        }

        if (curMangoLr == 0.0f && curMangoQuorum == 0.0f) {
            curMangoLr = oldRelValue;
            curMangoQuorum = oldQuorumValue;
        } else {
            curMangoLr += oldRelValue * oldRelDistWeight;
            curMangoQuorum += oldQuorumValue * oldQuorumDistWeight;
        }
        if (relSum == 0.0f) {
            relSum = 1.0f;
        }
        if (quorumSum == 0.0f) {
            quorumSum = 1.0f;
        }

        BestMangoLr = Max(BestMangoLr, curMangoLr / relSum);
        BestMangoQuorum = Max(BestMangoQuorum, curMangoQuorum / quorumSum);
        IsPerfectMatch = IsPerfectMatch || isPerfectMatch;
    }

    float GetCosRelevance(const TQueryWordInfos& infos, float textIdfNorm)
    {
        if (FastQuorumMode)
            return 0.0f;
        // скорректировать норму запроса
        QueryNorm += LastWordWeightSum * LastWordWeightSum;
        for (size_t i = 0; i < infos.size(); ++i) {
            if (QuorumHitTypes[i] == EQUAL_NO_MATCH) {
                float weight = infos[i].Weight;
                QueryNorm += weight * weight * HardRelevWeights[i];
            }
        }

        return Scalar / sqrt(QueryNorm * textIdfNorm);
    }

    void GetQuorum(const TQueryWordInfos& infos, const TWebDocQuorum& webQuorum, TRawTrFeatures& res)
    {
        if (!FastQuorumMode) {
            res.IsMegaForcedQuorum = true;
        }
        float quorumMatchSum = 0.f;
        float relevMatchSum = 0.f;
        float querySum = 0.f;

        for (size_t i = 0; i < QuorumHitTypes.size(); ++i) {
            float weight = QuorumRfs[i] * HardRelevWeights[i];
            float quorumClassWeight = GetQuorumClassWeight(QuorumHitTypes[i]);

            quorumMatchSum += weight * quorumClassWeight;
            querySum += weight;
            relevMatchSum += weight * GetFormClassWeight(QuorumHitTypes[i]);

            if (!FastQuorumMode) {
                res.IsMegaForcedQuorum = res.IsMegaForcedQuorum && (infos[i].IsStopWord || quorumClassWeight == 1.0f);
            }
        }


        float quorumBarjer = webQuorum.Get(WordCountNonStop);
        res.IsQuorum = quorumMatchSum >= quorumBarjer * querySum;
        res.QuorumRelevance = querySum > ZERO_EPS ? quorumMatchSum / querySum : 0.f;
        res.FastRelevance = querySum > ZERO_EPS ? relevMatchSum / querySum : 0.f;
        res.IsPerfectMatch = IsPerfectMatch;

        if (!FastQuorumMode) {
            res.IsForcedQuorum = res.IsQuorum && (HitStatistics[BEST_RELEV] >= 1 || HitStatistics[HIGH_RELEV] >= 1 || HitStatistics[MID_RELEV] >= 2);
            res.IsMegaForcedQuorum = res.IsMegaForcedQuorum && res.IsQuorum;
            res.IsQuorum = res.IsQuorum && BestMangoQuorum >= quorumBarjer * 0.85f;
            res.MangoLR = BestMangoLr;
            res.MangoQuorumRelevance = BestMangoQuorum;
        }
    }

    bool GetQuorumLite(const TWebDocQuorum& webQuorum, float& relev)
    {
        relev = BestMangoLr;
        return BestMangoQuorum >= webQuorum.Get(WordCountNonStop) * 0.85f;
    }
};
const float THitCounter::DEFAULT_KPM_COEF_VALUE = 1.0;


// ----- THitsBasedTr::TImpl -----

class THitsBasedTr::TImpl
{
    const TQueryWordInfos* QueryWordInfos;
    const TLinkInfo* LinkInfo;

    THitCounter TextHits;
    THitCounter TitleHits;
    bool FastQuorumMode;
    TWebDocQuorum Quorum;
public:
    void SetQuorum(float param);
    void Reset(const TLinkInfo* linkInfo, const TQueryWordInfos* queryWordInfos, const TProximityInfos* proxes, bool fastQuorumMode = false);
    void OnHit(const THitInfo &hitInfo);
    void CalcFeatures(TRawTrFeatures &features);
};

// ----- THitsBasedTr::TImpl -----

void THitsBasedTr::TImpl::SetQuorum(float param)
{
    Quorum = TWebDocQuorum(param);
}

void THitsBasedTr::TImpl::Reset(const TLinkInfo* linkInfo, const TQueryWordInfos* queryWordInfos, const TProximityInfos* proxes, bool fastQuorumMode /*= false*/)
{
    FastQuorumMode = fastQuorumMode;
    QueryWordInfos = queryWordInfos;
    LinkInfo = linkInfo;

    TextHits.Reset(*QueryWordInfos, FastQuorumMode, proxes, linkInfo);

    if (!FastQuorumMode) {
        TitleHits.Reset(*QueryWordInfos, FastQuorumMode, proxes, linkInfo);
    }
}

void THitsBasedTr::TImpl::OnHit(const THitInfo& hitInfo)
{
    TextHits.OnHit(hitInfo, (*QueryWordInfos)[hitInfo.QueryWordIndex]);

    if (!FastQuorumMode && hitInfo.Relev == HIGH_RELEV) {
        TitleHits.OnHit(hitInfo, (*QueryWordInfos)[hitInfo.QueryWordIndex]);
    }
}

void THitsBasedTr::TImpl::CalcFeatures(TRawTrFeatures& features)
{
    Zero(features);

    TextHits.GetQuorum(*QueryWordInfos, Quorum, features);

    if (FastQuorumMode) {
        features.IdfCosRel = features.MangoLR = features.MangoCombinedLR = features.FastRelevance;
        features.IsTitleQuorum = false;
        features.MangoTitleLR = 0.0f;
        return;
    }

    features.IsTitleQuorum = TitleHits.GetQuorumLite(Quorum, features.MangoTitleLR);

    features.IdfCosRel = TextHits.GetCosRelevance(*QueryWordInfos, LinkInfo->TextIdfNorm);
    features.MangoCombinedLR = features.MangoLR * 0.67f + features.IdfCosRel * 0.33f;
}

// ----- THitsBasedTr -----

THitsBasedTr::THitsBasedTr()
    : Impl(new TImpl())
{
    Impl->SetQuorum(0.5f);
}

THitsBasedTr::~THitsBasedTr()
{}

void THitsBasedTr::SetQuorum(float param)
{
    Impl->SetQuorum(param);
}

void THitsBasedTr::Reset(const TLinkInfo* linkInfo, const TQueryWordInfos* queryWordInfos, const TProximityInfos* proxes, bool fastQuorumMode /*= false*/)
{
    Impl->Reset(linkInfo, queryWordInfos, proxes, fastQuorumMode);
}

void THitsBasedTr::OnHit(const THitInfo& hitInfo)
{
    Impl->OnHit(hitInfo);
}

void THitsBasedTr::CalcFeatures(TRawTrFeatures& features)
{
    Impl->CalcFeatures(features);
}

} // NMango

