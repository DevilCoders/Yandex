#pragma once

#include <kernel/snippets/sent_info/sentword.h>

#include <util/generic/vector.h>

class TFactorStorage;

namespace NSnippets
{
    class IRestr;
    class TSentsMatchInfo;
    class TSingleSnip;
    class TSnip;
    class TWordSpanLen;
    class TInvalidWeight;

    class TSnipBuilder {
    private:
        typedef TSentMultiword TPos;
        struct TPart {
            TPos L;
            TPos R;
            TPart(const TPos& l, const TPos& r)
              : L(l)
              , R(r)
            {
            }
        };

        typedef TVector<TPart> TParts;

    private:
        const TWordSpanLen& SpanLen;
        const TSentsMatchInfo& SentsMatchInfo;
        const float MaxPartSize;
        TParts Parts;

    public:
        const float MaxSize;
    private:
        void ConvertParts(TVector<TSingleSnip>& p, size_t i, TPos l, TPos r);
        void ConvertParts(TVector<TSingleSnip>& p) const;
        bool JustGrow(size_t i, TPos l, TPos r);

    public:
        TSnipBuilder(const TSentsMatchInfo& match, const TWordSpanLen& wordSpanLen, float maxSize, float maxPartSize);

        void Reset();
        TSnip Get(double weight, TFactorStorage&& factors) const;
        TSnip Get(const TInvalidWeight& invalidWeight) const;
        bool Add(const TPos& a, const TPos& b);
        float GetSize() const;
        int GetPartsSize() const;
        TPos GetPartL(int part) const;
        TPos GetPartR(int part) const;

        void GlueTornSents();
        bool GrowLeftToSent();
        bool GrowLeftWordInSent();
        bool GrowRightToSent();
        bool GrowRightWordInSent();
        void GlueSents(const IRestr& restr);
        bool GrowRightMoreSent(const IRestr& restr);
    };

}

