#pragma once

#include "tr_base.h"

#include <kernel/mango/common/types.h>
#include <library/cpp/wordpos/wordpos.h>
#include <util/generic/utility.h>

namespace NMango
{

struct TProximityInfo
{
    TDistanceType DistanceType;
    int Beg, End;
    int Center;

    TProximityInfo() : DistanceType(DT_PHRASE), Beg(-64), End(64), Center(0) {}
    TProximityInfo(const TProximity& proximity)
        : DistanceType(proximity.DistanceType)
        , Beg(proximity.Beg)
        , End(proximity.End)
        , Center(proximity.Center)
    {
        if (proximity.Level == DOC_LEVEL) {
            --Beg; ++End;
            Beg <<= WORD_LEVEL_Bits;
            End <<= WORD_LEVEL_Bits;
            Center <<= WORD_LEVEL_Bits;
        }
    }
};

typedef TVector<TProximityInfo> TProximityInfos;

struct TRawTrFeatures
{
    float QuorumRelevance;
    float IdfCosRel;
    float MangoLR;
    float MangoQuorumRelevance;
    float MangoCombinedLR;
    float FastRelevance;
    float MangoTitleLR;
    bool  IsPerfectMatch:1;
    bool  IsQuorum:1;
    bool  IsForcedQuorum:1;
    bool  IsMegaForcedQuorum:1;
    bool  Unused:1;
    bool  IsTitleQuorum:1;

    TRawTrFeatures()
    {
        Zero(*this);
    }
};

class THitsBasedTr
{
public:
    THitsBasedTr();
    ~THitsBasedTr();
    void SetQuorum(float param);
    void Reset(const TLinkInfo* linkInfo, const TQueryWordInfos* queryWordInfos, const TProximityInfos* proxes, bool fastQuorumMode = false);
    void OnHit(const THitInfo& hitInfo);
    void CalcFeatures(TRawTrFeatures& features);
private:
    class TImpl;
    THolder<TImpl> Impl;
};

} // NMango
