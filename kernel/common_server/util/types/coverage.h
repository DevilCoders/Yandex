#pragma once

#include "interval.h"

#include <util/generic/map.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

#include <cmath>
#include <utility>

namespace NUtil {

template <class TType, class THandler>
class TCoverage {
private:
    using THandlers = TMap<TInterval<TType>, TVector<THandler>>;
    using iterator = typename THandlers::iterator;

private:
    THandlers Handlers;

public:
    using const_iterator = typename THandlers::const_iterator;

private:
    TType Min;
    TType Max;

private:
    enum EIntervalEnd {
        LeftEnd,
        RightEnd
    };

    void SplitHandlersInterval(TType position, EIntervalEnd end) {
        for (typename THandlers::iterator i = Handlers.begin(); i != Handlers.end(); ++i)
        if (i->first.Check(position)) {
            if (end == LeftEnd && position == i->first.GetMin())
                return;

            if (end == RightEnd && position == i->first.GetMax())
                return;

            const TInterval<TType> interval = i->first;
            const TVector<THandler> handlers = i->second;

            if (end == LeftEnd)
                position -= 1;

            typename THandlers::iterator next = Handlers.erase(i);
            Handlers.insert(next, std::make_pair(TInterval<TType>(interval.GetMin(), position), handlers));
            Handlers.insert(next, std::make_pair(TInterval<TType>(position + 1, interval.GetMax()), handlers));

            return;
        }
    }

    const TVector<TInterval<TType> > SplitIncomingInterval(const TInterval<TType>& interval) {
        TVector<TInterval<TType> > result;
        for (typename THandlers::iterator i = Handlers.begin(); i != Handlers.end(); ++i) {
            TType minCross = ::Max<TType>(i->first.GetMin(), interval.GetMin());
            TType maxCross = ::Min<TType>(i->first.GetMax(), interval.GetMax());

            if (minCross <= maxCross) {
                result.push_back(TInterval<TType>(minCross, maxCross));
            }
        }

        return result;
    }

    void DoAddHandler(const TInterval<TType>& interval, const THandler& handler) {
        for (typename THandlers::iterator i = Handlers.begin(); i != Handlers.end(); ++i) {
            if (i->first == interval) {
                i->second.push_back(handler);
                return;
            }
        }
        ythrow yexception() << "unexpected behaviour in DoAddHandler for incorrect interval";
    }

    double DoScale(const TType& position, const TType& newMin, const TType& newMax) {
        const double oldSize = Max - Min;
        const double newSize = newMax - newMin;
        const double scaling = newSize / oldSize;
        return newMin + (position - Min) * scaling;
    }
public:
    TCoverage(const TType& min, const TType& max)
        : Min(min)
        , Max(max)
    {
        Handlers.insert(Handlers.begin(), std::make_pair(TInterval<TType>(min, max), TVector<THandler>()));
    }

    inline void AddHandler(const TInterval<TType>& interval,
        const THandler& handler)
    {
        SplitHandlersInterval(interval.GetMin(), LeftEnd);
        SplitHandlersInterval(interval.GetMax(), RightEnd);

        const TVector<TInterval<TType> >& splittedIncomingInterval = SplitIncomingInterval(interval);
        for (typename TVector<TInterval<TType> >::const_iterator i = splittedIncomingInterval.begin();
            i != splittedIncomingInterval.end(); ++i)
            DoAddHandler(*i, handler);
    }

    bool DoesCoverInterval(const TInterval<TType>& interval) const {
        bool checking = false;
        for (typename THandlers::const_iterator i = Handlers.begin(); i != Handlers.end(); ++i) {
            if (i->first.Check(interval.GetMin()))
                checking = true;
            if (checking && i->second.size() == 0)
                return false;
            if (i->first.Check(interval.GetMax()))
                checking = false;
        }
        return true;
    }

    void Rescale(const TType& min, const TType& max) {
        if (max <= min)
            ythrow yexception() << "incorrect arguments: max <= min";

        typename THandlers::const_iterator i = Handlers.begin();
        Y_VERIFY(i != Handlers.end(), "unexpected behaviour");
        for (; i != Handlers.end(); ++i) {
            TType low = ceil(DoScale(i->first.GetMin(), min, max));
            TType high = floor(DoScale(i->first.GetMax(), min, max));
            i->first = TInterval<TType>(low, high);
        }
        Handlers.front().first.SetMin(min);
        Handlers.back().first.SetMax(max);
    }

    inline const_iterator begin() const {
        return Handlers.begin();
    }

    inline const_iterator end() const {
        return Handlers.end();
    }

    inline const_iterator lower_bound(TType value) const {
        return Handlers.lower_bound(TInterval<TType>(value));
    }

    inline const_iterator upper_bound(TType value) const {
        return Handlers.upper_bound(TInterval<TType>(value));
    }

    inline size_t size() const {
        return Handlers.size();
    }

    TType GetMin() const {
        return Min;
    }

    TType GetMax() const {
        return Max;
    }
};
}
