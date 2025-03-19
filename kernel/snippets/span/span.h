#pragma once

#include <util/system/defaults.h>
#include <util/generic/list.h>
#include <util/generic/utility.h>

namespace NSnippets
{

    struct TSpan
    {
        int First;
        int Last;

        TSpan():
            First(0),
            Last(0)
        {}

        TSpan(int f, int l)
        {
            if (f <= l) {
                First = f;
                Last = l;
            } else {
                First = l;
                Last = f;
            }
        }

        TSpan(const TSpan& a) = default;

        TSpan& operator=(const TSpan& a) = default;

        bool operator ==(const TSpan& a) const
        {
            return First == a.First && Last == a.Last;
        }

        bool operator <(const TSpan& a) const
        {
            return Last < a.First;
        }

        bool LeftTo(const TSpan& a) const
        {
            return Last + 1 == a.First;
        }

        bool Intersects(const TSpan& a) const
        {
            return
                (First <= a.First && a.First <= Last) ||
                (First <= a.Last && a.Last <= Last) ||
                (a.First <= First && Last <= a.Last);
        }

        bool Contains(int point) const
        {
            return First <= point && point <= Last;
        }

        bool Contains(const TSpan& a) const
        {
            return First <= a.First && a.Last <= Last;
        }

        void Merge(const TSpan& a)
        {
            First = Min(First, a.First);
            Last = Max(Last, a.Last);
        }

        size_t UnLen(const TSpan& a) const
        {
            size_t res = 0;
            if (a.First < First  && First <= a.Last)
                res +=  First - a.First;
            if (a.First <= Last && Last < a.Last)
                res += Last - a.Last;
            return res;
        }

        size_t Len() const
        {
            return Last - First + 1;
        }
    };

    typedef TList<TSpan> TSpans;
    typedef TSpans::iterator TSpanIt;
    typedef TSpans::const_iterator TSpanCIt;

}

