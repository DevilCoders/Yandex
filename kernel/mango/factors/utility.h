#pragma once

#include <util/system/defaults.h>

namespace NMango
{

class TAuthorAggregatedFactor
{
protected:
    float ValueFactor, ValueByAuthor;
public:
    TAuthorAggregatedFactor()
    {
        Reset();
    }

    inline void Reset()
    {
        ValueFactor = 0.0;
        ValueByAuthor = -1.0;
    }

    inline void OnAuthorStart()
    {
        ValueByAuthor = -1.0;
    }

    inline float OnAuthorFinish()
    {
        if (ValueByAuthor > 0.0) {
            ValueFactor += ValueByAuthor;
            return ValueByAuthor;
        }
        return 0.0f;
    }

    inline void OnNewValue(float value)
    {
        if (value > ValueByAuthor)
            ValueByAuthor = value;
    }

    inline float GetResult()
    {
        return ValueFactor;
    }
};


template<typename T, int N>
class TTopNAggregatedFactorImpl
{
    size_t Count, Seen;
    T Top[N];
    float CurValue;

public:
    TTopNAggregatedFactorImpl()
    {
        Reset();
    }

    inline void Reset()
    {
        CurValue = -1;
        Count = Seen = 0;
    }

    inline void OnAuthorStart()
    {
        CurValue = -1.0;
    }

    inline void OnNewValue(float value)
    {
        if (value > CurValue)
            CurValue = value;
    }

    inline void OnAuthorFinish()
    {
        if (CurValue <= 0)
            return;
        size_t pos;
        if (Count > 0 && CurValue < Top[Count - 1])
            pos = Count;
        else {
            pos = 0;
            while (pos < Count && Top[pos] > CurValue)
                ++pos;
        }

        if (pos == Count) {
            if (Count == N)
                return;
        } else {
            size_t amount = Count - pos;
            if (Count == N)
                --amount;
            memmove(&Top[pos + 1], &Top[pos], amount * sizeof(T));
        }
        Top[pos] = CurValue;
        ++Seen;
        if (Count < N)
            ++Count;
    }

    inline float GetResult()
    {
        size_t count = Count;
        // если запросов слишком мало - то отбросить трешак, худшие 50%
        if (Seen < N * 2)
            count = Min(count, (Seen + 1) / 2 + 1);
        if (count == 0)
            return 0.f;

        // взять среднее от Min(top10, top50%)
        float sum(0.0f);
        for (size_t i = 0; i < count; ++i)
            sum += Top[i];
        return sum / count;
    }
};

typedef TTopNAggregatedFactorImpl<float, 5> TTop5AggregatedFactor;

} // NMango
