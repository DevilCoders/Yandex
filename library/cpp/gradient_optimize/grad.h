#pragma once

#include "linear.h"
#include "vector_slice.h"
#include "golden_search.h"

#include <util/generic/algorithm.h>
#include <util/generic/ymath.h>
//#include <util/stream/output.h>

#include <cmath>

template <class T>
struct TId {
    using TType = T;
};

template <class TTFloat>
struct TDifferential {
    int I;
    TTFloat D;
    // D * dx_I

    TDifferential() = default;

    TDifferential(int i, TTFloat d)
        : I(i)
        , D(d)
    {
    }
};

template <class T>
void Reserve(TVectorType<T>& vec, int size) {
    //    if (vec.capacity() < size) {
    //        vec.reserve(size * 8);
    //    }
    vec.reserve(size);
}

template <class TTFloat>
struct TVariation {
    TTFloat X = TTFloat(0.);
    TVectorType<TDifferential<TTFloat>> DX;
    // X + DX; DX is kept sorted by I
    int R = 10;

    TVariation() = default;

    TVariation(TTFloat x) // implicit
        : X(x)
    {
    }

    TVariation(const TDifferential<TTFloat>& dx) // implicit
        : X(TTFloat())
        , DX(1, dx)
    {
    }

    TVariation(TTFloat x, int i, TTFloat d)
        : X(x)
        , DX(1, TDifferential<TTFloat>(i, d))
    {
        Y_ASSERT(i >= 0);
    }

    explicit operator TTFloat() const {
        return X;
    }

    void TryCompact() {
        if (DX.ysize() < R * 2) {
            //          Cout << "Compact skipped" << Endl;
            return;
        }
        Compact();
    }

    TVariation<TTFloat>& Compact() {
        Sort(DX.begin(), DX.end(),
             [](const TDifferential<TTFloat>& d1, const TDifferential<TTFloat>& d2) {
                 return d1.I < d2.I;
             });

        if (DX.ysize() > 0) {
            TVectorType<TDifferential<TTFloat>> dx;
            Reserve(dx, DX.size());

            TDifferential<TTFloat> d(-1, 0.);
            for (int i = 0; i < DX.ysize(); i++) {
                if (d.I != DX[i].I) {
                    if (d.I >= 0) {
                        dx.push_back(d);
                    }
                    d.I = DX[i].I;
                    d.D = 0.;
                }
                d.D += DX[i].D;
            }
            dx.push_back(d);

            DX = std::move(dx);
        }
        R = DX.ysize();
        return *this;
    }
};

template <class TTFloat>
TDifferential<TTFloat> operator-(const TDifferential<TTFloat>& dx) {
    return {dx.I, -dx.D};
}

template <class TTFloat>
TDifferential<TTFloat> operator*(typename TId<TTFloat>::TType c, const TDifferential<TTFloat>& dx) {
    return {dx.I, c * dx.D};
}

template <class TTFloat>
TDifferential<TTFloat>& operator*=(TDifferential<TTFloat>& dx, typename TId<TTFloat>::TType c) {
    dx.D *= c;
    return dx;
}

template <class TTFloat>
TDifferential<TTFloat> operator/(const TDifferential<TTFloat>& dx, typename TId<TTFloat>::TType c) {
    return {dx.I, dx.D / c};
}

template <class TTFloat>
TVariation<TTFloat> operator-(const TVariation<TTFloat>& x) {
    TVariation<TTFloat> y(-x.X);
    Reserve(y.DX, x.DX.size());
    for (int j = 0; j < x.DX.ysize(); j++) {
        y.DX.push_back(-x.DX[j]);
    }
    return y;
}

template <class TTFloat>
TVariation<TTFloat> operator-(TVariation<TTFloat>&& x) {
    x.X = -x.X;
    for (int j = 0; j < x.DX.ysize(); j++) {
        x.DX[j].D = -x.DX[j].D;
    }
    return x;
}

template <class TTFloat>
TVariation<TTFloat> operator+(const TVariation<TTFloat>& x1, const typename TId<TVariation<TTFloat>>::TType& x2) {
    TVariation<TTFloat> y = x1;
    return y += x2;
}

template <class TTFloat>
TVariation<TTFloat> operator+(typename TId<TTFloat>::TType x1, const TVariation<TTFloat>& x2) {
    return TVariation<TTFloat>(x1) + x2;
}

template <class TTFloat>
TVariation<TTFloat>& operator+=(TVariation<TTFloat>& x1, const typename TId<TVariation<TTFloat>>::TType& x2) {
    x1.X += x2.X;
    x1.R = Max(x1.R, x2.R);
    x1.DX.insert(x1.DX.end(), x2.DX.begin(), x2.DX.end());
    x1.TryCompact();
    return x1;
}

template <class TTFloat>
TVariation<TTFloat> operator-(const TVariation<TTFloat>& x1, const typename TId<TVariation<TTFloat>>::TType& x2) {
    TVariation<TTFloat> y = x1;
    return y -= x2;
}

template <class TTFloat>
TVariation<TTFloat> operator-(typename TId<TTFloat>::TType x1, const TVariation<TTFloat>& x2) {
    return TVariation<TTFloat>(x1) - x2;
}

template <class TTFloat>
TVariation<TTFloat>& operator-=(TVariation<TTFloat>& x1, const TVariation<TTFloat>& x2) {
    x1 += -x2;
    return x1;
}

template <class TTFloat>
TVariation<TTFloat>& operator*=(TVariation<TTFloat>& x1, const TVariation<TTFloat>& x2) {
    x1.R = Max(x1.R, x2.R);
    Reserve(x1.DX, x1.DX.size() + x2.DX.size());
    for (int j = 0; j < x1.DX.ysize(); j++) {
        x1.DX[j] *= x2.X;
    }
    for (int j = 0; j < x2.DX.ysize(); j++) {
        x1.DX.push_back(x1.X * x2.DX[j]);
    }
    x1.X *= x2.X;
    x1.TryCompact();
    return x1;
}

template <class TTFloat>
TVariation<TTFloat> operator*(const TVariation<TTFloat>& x1, const typename TId<TVariation<TTFloat>>::TType& x2) {
    //FIXME: copy/paste
    TVariation<TTFloat> y(x1.X * x2.X);
    y.R = Max(x1.R, x2.R);
    Reserve(y.DX, x1.DX.size() + x2.DX.size());
    for (int j = 0; j < x1.DX.ysize(); j++) {
        y.DX.push_back(x2.X * x1.DX[j]);
    }
    for (int j = 0; j < x2.DX.ysize(); j++) {
        y.DX.push_back(x1.X * x2.DX[j]);
    }
    y.TryCompact();
    return y;
}

template <class TTFloat>
TVariation<TTFloat> operator*(TVariation<TTFloat>&& x1, const typename TId<TVariation<TTFloat>>::TType& x2) {
    return x1 *= x2;
}

template <class TTFloat>
TVariation<TTFloat> operator*(const TVariation<TTFloat>& x1, typename TId<TVariation<TTFloat>>::TType&& x2) {
    return x2 *= x1;
}

template <class TTFloat>
TVariation<TTFloat> operator*(TVariation<TTFloat>&& x1, typename TId<TVariation<TTFloat>>::TType&& x2) {
    if (x1.DX.size() > x2.DX.size()) {
        return x1 *= x2;
    } else {
        return x2 *= x1;
    }
}

template <class TTFloat>
TVariation<TTFloat> operator*(typename TId<TTFloat>::TType x1, const TVariation<TTFloat>& x2) {
    return TVariation<TTFloat>(x1) * x2;
}

template <class TTFloat>
TVariation<TTFloat> operator/(const TVariation<TTFloat>& x1, const typename TId<TVariation<TTFloat>>::TType& x2) {
    TTFloat x2Inv = 1. / x2.X;
    TTFloat x1x2Inv2 = x1.X * x2Inv * x2Inv;
    TVariation<TTFloat> y(x1.X * x2Inv);
    y.R = Max(x1.R, x2.R);
    Reserve(y.DX, x1.DX.size() + x2.DX.size());
    for (int j = 0; j < x1.DX.ysize(); j++) {
        y.DX.push_back(x2Inv * x1.DX[j]);
    }
    for (int j = 0; j < x2.DX.ysize(); j++) {
        y.DX.push_back(-x1x2Inv2 * x2.DX[j]);
    }
    y.TryCompact();
    return y;
}

template <class TTFloat>
TVariation<TTFloat> operator/(typename TId<TTFloat>::TType x1, const TVariation<TTFloat>& x2) {
    return TVariation<TTFloat>(x1) / x2;
}

template <class TTFloat>
TVariation<TTFloat>& operator/=(TVariation<TTFloat>& x1, const typename TId<TVariation<TTFloat>>::TType& x2) {
    x1 = x1 / x2;
    return x1;
}

template <class TTFloat>
bool operator>(const TVariation<TTFloat>& x1, const typename TId<TVariation<TTFloat>>::TType& x2) {
    return x1.X > x2.X;
}

template <class TTFloat>
bool operator<(const TVariation<TTFloat>& x1, const typename TId<TVariation<TTFloat>>::TType& x2) {
    return x1.X < x2.X;
}

template <class TTFloat>
bool operator<=(const TVariation<TTFloat>& x1, const typename TId<TVariation<TTFloat>>::TType& x2) {
    return x1.X <= x2.X;
}

template <class TTFloat>
bool operator>=(const TVariation<TTFloat>& x1, const typename TId<TVariation<TTFloat>>::TType& x2) {
    return x1.X >= x2.X;
}

template <class TTFloat>
bool operator==(const TVariation<TTFloat>& x1, const typename TId<TVariation<TTFloat>>::TType& x2) {
    return x1.X == x2.X;
}

template <class TTFloat>
TVariation<TTFloat> fabs(const TVariation<TTFloat>& x) {
    return x < 0. ? -x : x;
}

template <class TTFloat>
TVariation<TTFloat> exp(TVariation<TTFloat>&& x) {
    x.X = exp(x.X);
    for (int i = 0; i < x.DX.ysize(); i++) {
        x.DX[i] *= x.X;
    }
    return x;
}

template <class TTFloat>
TVariation<TTFloat> exp(const TVariation<TTFloat>& x) {
    auto y = x;
    return exp(std::move(y));
}

template <class TTFloat>
TVariation<TTFloat> log(TVariation<TTFloat>&& x) {
    TTFloat x1 = 1. / x.X;
    x.X = log(x.X);
    for (int i = 0; i < x.DX.ysize(); i++) {
        x.DX[i] *= x1;
    }
    return x;
}

template <class TTFloat>
TVariation<TTFloat> log(const TVariation<TTFloat>& x) {
    auto y = x;
    return log(std::move(y));
}

template <class T>
T pow(T x, T y) {
    return exp(log(x) * y);
}

template <class TTFloat>
TVariation<TTFloat> sqrt(const TVariation<TTFloat>& x) {
    auto y = x;
    return sqrt(std::move(y));
}

template <class TTFloat>
TVariation<TTFloat> sqrt(TVariation<TTFloat>&& x) {
    x.X = sqrt(x.X);
    TTFloat y = 0.5 / x.X;
    for (int i = 0; i < x.DX.ysize(); i++) {
        x.DX[i] *= y;
    }
    return x;
}

template <class T>
T logistic(T x) {
    if (x > T(0.))
        return T(1.) / (T(1.) + exp(-x));
    else
        return exp(x) / (T(1.) + exp(x));
}

template <class T>
T logit(T p) {
    return log(p / (T(1.) - p));
}

namespace NTraits {
    template <class TTContainer>
    struct TContainerElement {};

    template <class T>
    struct TContainerElement<TVectorType<T>> {
        typedef T TResult;
    };

    template <class T>
    struct TContainerElement<TVectorSlice<T>> {
        typedef T TResult;
    };
}

template <class TTVector>
TVectorType<typename NTraits::TContainerElement<TTVector>::TResult> VectorLogistic(const TTVector& v) {
    typedef typename NTraits::TContainerElement<TTVector>::TResult TTFloat;
    TTFloat v0 = v[0];
    for (int i = 1; i < v.ysize(); i++) {
        if (v0 < v[i])
            v0 = v[i];
    }
    TTFloat q = 0.;
    TVectorType<TTFloat> u(v.ysize());
    for (int i = 0; i < v.ysize(); i++) {
        u[i] = exp(v[i] - v0);
        q += u[i];
    }
    for (int i = 0; i < v.ysize(); i++) {
        u[i] /= q;
    }

    return u;
}

//template <class TTVector>
//TVectorType<typename NTraits::TContainerElement<TTVector>::TResult> VectorLogit(const TTVector& v) {
//    typedef typename NTraits::TContainerElement<TTVector>::TResult TTFloat;

// also works!
template <template <class...> class TTVector, class TTFloat, class... R>
TVectorType<TTFloat> VectorLogit(const TTVector<TTFloat, R...>& v) {
    TTFloat s = v[0];
    for (int i = 1; i < v.ysize(); i++) {
        s += v[i];
    }
    TTFloat logS = log(s);
    TVectorType<TTFloat> u(v.ysize());
    for (int i = 0; i < v.ysize(); i++) {
        u[i] = log(v[i]) - logS;
    }

    return u;
}

template <class TTFloat>
TVariation<TTFloat> erf(const TVariation<TTFloat>& x) {
    TVariation<TTFloat> y(erf(x.X));
    TTFloat z = M_2_SQRTPI * exp(-Sqr(x.X));
    y.DX.resize(x.DX.size());
    for (int j = 0; j < x.DX.ysize(); j++) {
        y.DX[j] = z * x.DX[j];
    }
    return y;
}

template <class TTFloat, class TTFunc>
TVariation<TTFloat> NumericDerivative(TTFunc f, const TVectorType<TTFloat>& x0) {
    TVariation<TTFloat> y(f(x0));
    y.DX.reserve(x0.size());
    for (int i = 0; i < x0.ysize(); i++) {
        TVectorType<TTFloat> x = x0;
        x[i] += 1e-10;
        TTFloat y2 = f(x);
        x[i] -= 2e-10;
        TTFloat y1 = f(x);
        y.DX.push_back(TDifferential<TTFloat>(i, (y2 - y1) / 2e-10));
    }
    return y;
}

template <class TTFloat>
IOutputStream& operator<<(IOutputStream& os, const TVariation<TTFloat>& x) {
    os << x.X;
    for (int j = 0; j < x.DX.ysize(); j++) {
        os << " + " << x.DX[j].D << "dx_" << x.DX[j].I;
    }
    return os;
}

template <class TTData>
struct TVariationGradient {
    TTData& Data;

    TVariationGradient(TTData& data)
        : Data(data)
    {
    }

    TVectorType<double> operator()(const TVectorType<double>& x0) {
        TVectorType<TVariation<double>> x(x0.size());
        for (int i = 0; i < x0.ysize(); i++) {
            x[i] = TVariation<double>(x0[i], i, 1.);
        }
        TVariation<double> y = Data(x);
        y.Compact();
        TVectorType<double> z(x0.size(), 0.);
        for (int j = 0; j < y.DX.ysize(); j++) {
            z[y.DX[j].I] = y.DX[j].D;
        }
        return z;
    }
};

template <class TTData>
TVectorType<double> VariationGradient(TTData& data, const TVectorType<double>& x0) {
    TVariationGradient<TTData> grad(data);
    return grad(x0);
}

struct TConjugateGradientDescendOptions {
    bool Verbose = false;
};

template <class TTData, class TTDataGradient = TVariationGradient<TTData>>
struct TConjugateGradientDescend {
    TTData& Data;
    TTDataGradient Grad;
    TVectorType<double>& X;
    int K = 0;
    double T = 1.e-10;
    TVectorType<double> S;
    TVectorType<double> R;

    TConjugateGradientDescendOptions Options;

    TConjugateGradientDescend(TTData& data, TVectorType<double>& x, TConjugateGradientDescendOptions options = TConjugateGradientDescendOptions())
        : Data(data)
        , Grad(Data)
        , X(x)
        , Options(options)
    {
    }

    bool Step() {
        TVectorType<double> r = Grad(X);
        r = -1. * r;
        double w = 0.;
        double grad_squared = R % R;
        if ((K > 0) && grad_squared == 0) {
            Cerr << "CGD: cannot optimize, zero gradient!" << Endl;
        }
        if ((K > 0) && (grad_squared > 0)) {
            w = Max(0., (r % (r - R)) / grad_squared);
        }

        if (w <= 0.) {
            S = r; // S not defined yet at K == 0
        } else {
            S = r + w * S;
        }

        double prevStep = 0.;
        double mult = 1.61803398874989;
        double step = 1.e-10; // T / mult
        double score = Data(X);
        int iters = 2;
        while (step < 10.) {
            double newScore = Data(X + step * mult * S);
            if (!(newScore <= score)) {
                break;
            }
            prevStep = step;
            step *= mult;
            score = newScore;
            iters++;
        }
        auto f = [&](double q) -> double {
            auto z = X + q * S; // X - log(q) * S
            return Data(z);
        };
        T = GoldenSearch(f, prevStep, step * mult, 0, iters);
        //T = GoldenSearch(f, 0, 1, 0, iters);

        if (Options.Verbose) {
            Cerr << "CGD: t=" << T << Endl;
        }

        R = r;
        if (!(Data(X) >= Data(X + T * S))) {
            if (K == 0) {
                if (Options.Verbose) {
                    Cerr << "CGD: done" << Endl;
                }
                return false;
            }
            K = 0; // smth went wrong => reset
            if (Options.Verbose) {
                Cerr << "CGD: reset" << Endl;
            }
            return Step();
        } else {
            X = X + T * S;
            K++;
            if (K >= 10) {
                K = 0;
            }
            return true;
        }
    }

    void Step(int n) {
        for (int i = 0; i < n; i++) {
            Step();
        }
    }
};
