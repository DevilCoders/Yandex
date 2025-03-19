#pragma once

#include <util/generic/vector.h>
#include <kernel/qtree/richrequest/proxim.h>

namespace NMango {

    struct TQueryWordInfo
    {
        float Weight;
        float DocFreq;
        bool  IsStopWord;

        TQueryWordInfo(float weight, float docFreq, bool isStopWord)
            : Weight(weight)
            , DocFreq(docFreq)
            , IsStopWord(isStopWord)
        {}
    };

    typedef TVector<TQueryWordInfo> TQueryWordInfos;

    struct THitInfo
    {
        ui16 WordPosition;
        ui16 SentenceNumber;
        ui32 GlobalWordPosition;
        EFormClass Form;
        RelevLevel Relev;
        int QueryWordIndex;

        THitInfo()
        {
            Zero(*this);
        }

        bool friend operator <(const THitInfo& a, const THitInfo& b)
        {
            if (a.GlobalWordPosition != b.GlobalWordPosition) {
                return a.GlobalWordPosition < b.GlobalWordPosition;
            }
            if (a.QueryWordIndex != b.QueryWordIndex) {
                return a.QueryWordIndex < b.QueryWordIndex;
            }
            if (a.Relev != b.Relev) {
                return a.Relev < b.Relev;
            }
            return a.Form < b.Form;
        }

        bool friend operator <<(const THitInfo& a, const THitInfo& b)
        {
            if (a.GlobalWordPosition != b.GlobalWordPosition) {
                return a.GlobalWordPosition < b.GlobalWordPosition;
            }
            return a.QueryWordIndex < b.QueryWordIndex;
        }
    };
}

