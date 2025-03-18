#pragma once

#include "linear.h"
#include "grad.h"

template <class TTRegularization>
struct TProximity;

struct TNullRegularization {
    template <class TTFloat>
    TTFloat operator()(const TVectorType<TTFloat>&) {
        return 0.;
    }
};

template <>
struct TProximity<TNullRegularization> {
    TProximity(const TNullRegularization&) {
    }
    void operator()(TVectorType<double>&, double) {
        // prox(x, h) = x
    }
};

struct TL1Regularization {
    double Lambda;

    TL1Regularization(double lambda = 1.)
        : Lambda(lambda)
    {
    }

    template <class TTFloat>
    TTFloat operator()(const TVectorType<TTFloat>& x) {
        TTFloat l1 = 0.;
        for (int i = 0; i < x.ysize(); i++) {
            l1 += fabs(x[i]);
        }
        return Lambda * l1;
    }
};

template <>
struct TProximity<TL1Regularization> {
    TL1Regularization& H;
    TProximity(TL1Regularization& h)
        : H(h)
    {
    }

    void operator()(TVectorType<double>& x, double h) {
        h *= H.Lambda;
        for (int i = 0; i < x.ysize(); i++) {
            if (x[i] > h) {
                x[i] -= h;
            } else if (x[i] < -h) {
                x[i] += h;
            } else {
                x[i] = 0.;
            }
        }
    }
};

template <class TTCData, class TTGrad, class TTSData>
// f(x) = g(x) + h(x). g(x) is continuous, h(x) has simple proximity operator
struct TGenericFista {
    TVectorType<double>& X;
    TVectorType<double> X0;
    double T = 1.;
    int K = 0;
    int N = X.ysize();

    TTCData& G;
    TTGrad GGrad;
    TTSData H;
    TProximity<TTSData> HProx;

    TGenericFista(TVectorType<double>& x, TTCData& g, TTSData h = TTSData())
        : X(x)
        , X0(X)
        , G(g)
        , GGrad(G)
        , H(h)
        , HProx(H)
    {
    }

    void Step(int n = 1) {
        for (int i = 0; i < n; i++) {
            double a = (K - 2.) / (K + 1.);
            TVectorType<double> y = X + a * (X - X0);
            K++;
            X0 = X;

            auto dg = GGrad(y);
            for (;;) {
                X = y - T * dg;
                HProx(X, T);
                auto dxy = X - y;
                if (G(X) <= G(y) + dxy % dg + 1. / (2. * T) * (dxy % dxy))
                    break;
                T = 0.95 * T;
            }
            //Cerr << "Fista: T=" << T << Endl;
            // Cout << "X: " << X << Endl;
        }
    }
};

template <class TTCData>
using TSimpleFista = TGenericFista<TTCData, TVariationGradient<TTCData>, TNullRegularization>;

template <class TTCData>
using TL1Fista = TGenericFista<TTCData, TVariationGradient<TTCData>, TL1Regularization>;
