#pragma once

#include <library/cpp/pop_count/popcount.h>

#include <util/generic/map.h>
#include <util/generic/yexception.h>
#include <util/generic/algorithm.h>
#include <util/generic/vector.h>
#include <util/generic/ylimits.h>

#include <util/system/defaults.h>
#include <util/system/yassert.h>

#include <math.h>

template <class T>
Y_FORCE_INLINE ui32 HammingDistance(T N1, T N2) {
    return PopCount<T>(N1 ^ N2);
}

template <class T>
Y_FORCE_INLINE float DifferenceRatio(T N1, T N2) {
    const float first = fabs((float)N1);
    const float second = fabs((float)N2);
    const float max = Max(first, second);
    if (max > 0.0f) {
        return fabs(first - second) / max;
    } else {
        return 0.0f;
    }
}

inline ui64 SetBit(ui64 simhash, ui16 bit) {
    ui64 b = ((ui64)1) << bit;
    return simhash ^ b;
}

inline void MakeNeighbors(const ui64 simhash, ui32 dst, TVector<ui64>& res) {
    res.clear();
    if (dst == 1) {
        for (size_t k = 0; k < 64; ++k) {
            ui64 r = SetBit(simhash, k);
            res.push_back(r);
        }
    } else if (dst == 2) {
        for (size_t j = 0; j < 63; ++j) {
            for (size_t k = j + 1; k < 64; ++k) {
                ui64 r = SetBit(simhash, j);
                r = SetBit(r, k);
                res.push_back(r);
            }
        }
    } else if (dst == 3) {
        for (size_t i = 0; i < 62; ++i) {
            for (size_t j = i + 1; j < 63; ++j) {
                for (size_t k = j + 1; k < 64; ++k) {
                    ui64 r = SetBit(simhash, i);
                    r = SetBit(r, j);
                    r = SetBit(r, k);
                    res.push_back(r);
                }
            }
        }
    }
    for (size_t i = 0; i < res.size(); ++i) {
        if (PopCount(res[i] ^ simhash) != dst) {
            ythrow yexception() << "wrong simhash generation";
        }
    }
}

template <class TValue>
class TMinMaxTemplate {
public:
    TMinMaxTemplate()
        : max()
        , min()
        , hasVal(false)
    {
        Clear();
    }

public:
    TValue GetMax() const {
        return max;
    }

    TValue GetMin() const {
        return min;
    }

    void Insert(const TValue& Value) {
        if (hasVal) {
            max = Max(max, Value);
            min = Min(min, Value);
        } else {
            min = max = Value;
        }
        hasVal = true;
    }

    template <class T>
    void Merge(const TMinMaxTemplate<T>& Other) {
        if (Other.HasVal()) {
            Insert(Other.GetMin());
            Insert(Other.GetMax());
        }
    }

    void Clear() {
        max = TValue();
        min = TValue();
        hasVal = false;
    }

    bool HasVal() const {
        return hasVal;
    }

private:
    TValue max;
    TValue min;
    bool hasVal;
};

using TI8MinMax = TMinMaxTemplate<i8>;
using TUI8MinMax = TMinMaxTemplate<ui8>;
using TI16MinMax = TMinMaxTemplate<i16>;
using TUI16MinMax = TMinMaxTemplate<ui16>;
using TI32MinMax = TMinMaxTemplate<i32>;
using TUI32MinMax = TMinMaxTemplate<ui32>;
using TI64MinMax = TMinMaxTemplate<i64>;
using TUI64MinMax = TMinMaxTemplate<ui64>;
using TDoubleMinMax = TMinMaxTemplate<double>;
using TLongDoubleMinMax = TMinMaxTemplate<long double>;

template <class TValue, class TInternalCoeff = long double>
class TMeanTemplate {
public:
    TMeanTemplate()
        : value()
        , count(0)
    {
    }

public:
    TValue GetMean() const {
        return value;
    }

    ui64 GetCount() const {
        return count;
    }

    void Insert(const TValue& Value, const ui32 NumInValue = 1) {
        const ui64 oldCount = count;
        count += NumInValue;
        const TInternalCoeff n_m_one_d_n = ((TInternalCoeff)(oldCount)) / ((TInternalCoeff)count);
        const TInternalCoeff one_d_n = ((TInternalCoeff)1) / ((TInternalCoeff)count);

        value = (TValue)((n_m_one_d_n * ((TInternalCoeff)value)) + (one_d_n * ((TInternalCoeff)Value)));
    }

    template <class TV, class TC>
    void Merge(const TMeanTemplate<TV, TC>& Other) {
        if (Other.GetCount() == 0) {
            return;
        }

        TInternalCoeff ks = ((TInternalCoeff)Other.count) * ((TInternalCoeff)Other.value);
        TInternalCoeff ms = ((TInternalCoeff)count) * ((TInternalCoeff)value);
        TInternalCoeff km = (TInternalCoeff)Other.count + (TInternalCoeff)count;

        value = (TValue)((ks + ms) / km);
        count += Other.count;
    }

    void Clear() {
        value = TValue();
        count = 0;
    }

private:
    TValue value;
    ui64 count;
};

using TDoubleMean = TMeanTemplate<double>;
using TLongDoubleMean = TMeanTemplate<long double>;

template <class TValue, class TInternalCoeff = long double>
class TMeanDispTemplate {
public:
    TMeanDispTemplate()
        : mean()
        , square_mean()
    {
    }

public:
    TValue GetMean() const {
        return mean.GetMean();
    }

    TValue GetSquareMean() const {
        return square_mean.GetMean();
    }

    TValue GetDispersion() const {
        return square_mean.GetMean() - (mean.GetMean()) * (mean.GetMean());
    }

    ui64 GetCount() const {
        return mean.GetCount();
    }

    void Insert(const TValue& Value, const ui32 NumInValue = 1) {
        if (NumInValue == 0) {
            return;
        }
        mean.Insert(Value, NumInValue);
        square_mean.Insert(Value * Value / (TValue)NumInValue, NumInValue);
    }

    template <class TV, class TC>
    void Merge(const TMeanDispTemplate<TV, TC>& Other) {
        if (Other.GetCount() == 0) {
            return;
        }

        mean.Merge(Other.mean);
        square_mean.Merge(Other.square_mean);
    }

    void Clear() {
        mean.Clear();
        square_mean.Clear();
    }

private:
    TMeanTemplate<TValue, TInternalCoeff> mean;
    TMeanTemplate<TValue, TInternalCoeff> square_mean;
};

using TDoubleMeanDisp = TMeanDispTemplate<double>;
using TLongDoubleMeanDisp = TMeanDispTemplate<long double>;

template <class TValue, class TInternalCoeff = long double>
class TCovarianceTemplate {
public:
    explicit TCovarianceTemplate(ui32 Dim)
        : means(0)
        , crossMeans(0)
    {
        if (Dim == 0) {
            ythrow yexception() << "Dim must be > 0";
        }
        means.resize(Dim, TMeanDispTemplate<TValue, TInternalCoeff>());
        crossMeans.resize(Dim - 1, TVector<TMeanTemplate<TValue, TInternalCoeff>>(0));
        for (ui32 i = 0; i < Dim - 1; ++i) {
            crossMeans[i].resize(Dim - i - 1, TMeanTemplate<TValue, TInternalCoeff>());
        }
    }

    ui32 GetDim() const {
        return (ui32)means.size();
    }

    ui32 GetCount() const {
        return means[0].GetCount();
    }

    TValue GetMean(ui32 I) const {
        Y_VERIFY((I < GetDim()), "I must be < Dim");
        return means[I].GetMean();
    }

    TValue GetDispersion(ui32 I) const {
        Y_VERIFY((I < GetDim()), "I must be < Dim");
        return means[I].GetDispersion();
    }

    TValue GetCovariance(ui32 I, ui32 J) const {
        Y_VERIFY((I < GetDim()), "I must be < Dim");
        Y_VERIFY((J < GetDim()), "J must be < Dim");

        if (I == J) {
            return GetDispersion(I);
        }
        if (I > J) {
            DoSwap(I, J);
        }
        return crossMeans[I].at((J - I) - 1).GetMean() - means[I].GetMean() * means[J].GetMean();
    }

    TValue GetCorrelation(ui32 I, ui32 J) const {
        TValue di = GetDispersion(I);
        TValue dj = GetDispersion(J);
        if ((di == 0) || (dj == 0)) {
            return 0;
        }
        return GetCovariance(I, J) / (sqrt(di) * sqrt(dj));
    }

    template <class T>
    void Insert(const TVector<T>& Values) {
        Y_VERIFY(((ui32)Values.size() == GetDim()), "Wrong input dim");
        for (size_t i = 0; i < Values.size(); ++i) {
            means[i].Insert((TValue)Values[i]);
            for (size_t j = i + 1; j < Values.size(); ++j) {
                crossMeans[i][j - i - 1].Insert((TValue)(Values[i] * Values[j]));
            }
        }
    }

    template <class TV, class TC>
    void Merge(const TCovarianceTemplate<TV, TC>& Other) {
        if (Other.GetCount() == 0) {
            return;
        }

        Y_VERIFY((GetDim() == Other.GetDim()), "Wrong other dim");
        for (ui32 i = 0; i < GetDim(); ++i) {
            means[i].Merge(Other.means[i]);
            for (ui32 j = 0; j < GetDim() - i - 1; ++j) {
                crossMeans[i][j].Merge(Other.crossMeans[i][j]);
            }
        }
    }

    void Clear() {
        for (ui32 i = 0; i < GetDim(); ++i) {
            means[i].Clear();
            for (ui32 j = 0; j < GetDim() - i - 1; ++j) {
                crossMeans[i][j].Clear();
            }
        }
    }

private:
    TVector<TMeanDispTemplate<TValue, TInternalCoeff>> means;
    TVector<TVector<TMeanTemplate<TValue, TInternalCoeff>>> crossMeans;
};

using TDoubleCovariance = TCovarianceTemplate<double>;
using TLongDoubleCovariance = TCovarianceTemplate<long double>;
