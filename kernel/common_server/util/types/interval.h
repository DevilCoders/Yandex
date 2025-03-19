#pragma once

#include <util/generic/algorithm.h>
#include <util/generic/typetraits.h>
#include <util/generic/yexception.h>
#include <util/string/cast.h>
#include <util/generic/utility.h>
#include <util/generic/vector.h>
#include <util/string/vector.h>
#include <util/ysaveload.h>

namespace NUtil {

template <class TType>
class TInterval {
private:
    using TSelf = TInterval<TType>;
    TType Min = 0;
    TType Max = 0;

    template <bool RightBorderUsage>
    bool IntersectionImpl(const TInterval<TType>& interval) const {
        if (RightBorderUsage) {
            return Max >= interval.Min && interval.Max >= Min;
        } else {
            return Max > interval.Min && interval.Max > Min;
        }
    }

    template <bool RightBorderUsage>
    bool IntersectionImpl(const TInterval<TType>& interval, TInterval<TType>& result) const {
        if (Min > interval.Min) {
            result.Min = Min;
        } else {
            result.Min = interval.Min;
        }
        if (Max < interval.Max) {
            result.Max = Max;
        } else {
            result.Max = interval.Max;
        }
        if (RightBorderUsage) {
            return result.Max >= result.Min;
        } else {
            return result.Max > result.Min;
        }
    }

public:
    TInterval() {
    }

    static TSelf SafeConstruct(const TType& v1, const TType& v2) {
        if (v1 < v2) {
            return TSelf(v1, v2);
        } else {
            return TSelf(v2, v1);
        }
    }

    inline TInterval(typename TTypeTraits<TType>::TFuncParam min,
        typename TTypeTraits<TType>::TFuncParam max)
        : Min(min)
        , Max(max)
    {
        if (Min > Max) {
            ythrow yexception() << "Incorrect interval: " << ToString();
        }
    }

    inline TInterval(typename TTypeTraits<TType>::TFuncParam value)
        : Min(value)
        , Max(value)
    {
    }

    std::pair<TType, TType> GetPair() const
    {
        return std::make_pair(Min, Max);
    }

    TType GetLength() const
    {
        return Max - Min;
    }

    inline typename TTypeTraits<TType>::TFuncParam GetMin() const
    {
        return Min;
    }

    inline TSelf& SetMin(typename TTypeTraits<TType>::TFuncParam value)
    {
        Min = value;
        return *this;
    }

    inline typename TTypeTraits<TType>::TFuncParam GetMax() const
    {
        return Max;
    }

    inline TSelf& SetMax(typename TTypeTraits<TType>::TFuncParam value)
    {
        Max = value;
        return *this;
    }

    inline bool Check(typename TTypeTraits<TType>::TFuncParam value) const
    {
        return value >= Min && value <= Max;
    }

    inline bool CheckNoBorders(typename TTypeTraits<TType>::TFuncParam value) const
    {
        return value > Min && value < Max;
    }

    inline bool CheckLeftBorder(typename TTypeTraits<TType>::TFuncParam value) const
    {
        return value >= Min && value < Max;
    }

    inline bool CheckRightBorder(typename TTypeTraits<TType>::TFuncParam value) const
    {
        return value > Min && value <= Max;
    }

    inline TString ToString() const
    {
        return ::ToString(Min) + "-" + ::ToString(Max);
    }

    inline bool operator < (const TInterval<TType>& interval) const
    {
        return Max < interval.Min;
    }

    inline bool operator > (const TInterval<TType>& interval) const
    {
        return Min > interval.Max;
    }

    inline bool operator == (const TInterval<TType>& interval) const
    {
        return Min == interval.Min && Max == interval.Max;
    }

    inline bool operator != (const TInterval<TType>& interval) const
    {
        return !(*this == interval);
    }

public:
    bool IsContainedIn(const TInterval<TType>& interval) const {
        return (interval.GetMax() >= GetMax() && interval.GetMax() >= GetMin() &&
            interval.GetMin() <= GetMax() && interval.GetMin() <= GetMin()
            );
    }

    bool Join(const TInterval<TType>& interval) {
        if (Intersection(interval) || FollowedBy(interval) || interval.FollowedBy(*this)) {
            SetMin(::Min<TType>(GetMin(), interval.GetMin()));
            SetMax(::Max<TType>(GetMax(), interval.GetMax()));
            return true;
        }
        return false;

    }

    void Extend(const TType& value) {
        SetMin(::Min<TType>(GetMin(), value));
        SetMax(::Max<TType>(GetMax(), value));
    }

    bool FollowedBy(const TInterval<TType>& interval) const {
        return GetMax() == interval.GetMin() - 1;
    }

    bool Intersection(const TInterval<TType>& interval) const {
        return IntersectionImpl<true>(interval);
    }

    bool Intersection(const TInterval<TType>& interval, TInterval<TType>& result) const {
        return IntersectionImpl<true>(interval, result);
    }

    bool IntersectionNoRight(const TInterval<TType>& interval) const {
        return IntersectionImpl<false>(interval);
    }

    bool IntersectionNoRight(const TInterval<TType>& interval, TInterval<TType>* result) const {
        return IntersectionImpl<false>(interval, *result);
    }

    static bool Parse(const TString& intervalString, TInterval<TType>& result, const char* splitSymbol = "-") {
        TVector<TString> parts = SplitString(intervalString, splitSymbol);
        return parts.size() == 2 && TryFromString(parts[0], result.Min) && TryFromString(parts[1], result.Max);
    }

    static TVector<TInterval<TType>> Merge(const TVector<TInterval<TType>>& intervals) {
        bool keyModif = true;
        TVector<TInterval<TType>> result = intervals;
        while (keyModif) {
            keyModif = false;
            for (ui32 i = 0; i < result.size(); ++i) {
                for (ui32 j = i + 1; j < result.size();) {
                    if (result[i].Join(result[j])) {
                        result.erase(result.begin() + j);
                        keyModif = true;
                    } else
                        ++j;
                }
            }
        }
        return result;
    }

    static TInterval<TType> Join(const TInterval<TType>& one, const TInterval<TType>& two) {
        TInterval<TType> first = one;
        TInterval<TType> second = two;
        if (first.GetMin() > second.GetMin() ||
           (first.GetMin() == second.GetMin() && first.GetMax() > second.GetMax()))
            DoSwap(first, second);

        if (first.GetMax() < second.GetMin() - 1)
            ythrow yexception() << "cannot join " << first.ToString() << " and " << second.ToString();

        return TInterval<TType>(first.GetMin(), second.GetMax());
    }

    bool Merge(const TInterval<TType>& additional) {
        if (Min <= additional.Max && additional.Min <= Max) {
            Max = ::Max(additional.Max, Max);
            Min = ::Min(additional.Min, Min);
            return true;
        }
        return false;
    }
};
}

template <class TType>
class TSerializer<NUtil::TInterval<TType>> {
    using TInterval = NUtil::TInterval<TType>;
    using TPair = std::pair<TType, TType>;

public:
    static inline void Save(IOutputStream* rh, const TInterval& i) {
        ::Save(rh, i.GetPair());
    }

    static inline void Load(IInputStream* rh, TInterval& i) {
        TPair p;
        ::Load(rh, p);
        i = TInterval(p.first, p.second);
    }

    template <class TStorage>
    static inline void Load(IInputStream* rh, TInterval& i, TStorage& pool) {
        TPair p;
        ::Load(rh, p, pool);
        i = TInterval(p.first, p.second);
    }
};

// compatibility typedef
template <class T>
using TInterval = NUtil::TInterval<T>;
