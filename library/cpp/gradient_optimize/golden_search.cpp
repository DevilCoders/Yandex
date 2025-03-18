#include <util/generic/ymath.h>

#include "golden_search.h"

static const double Phi1 = 0.618033988749895; // 2. / (1. + sqrt(5.));

template <class TFunc>
struct TGoldenSearch {
    TFunc F;
    double A, B, Ca, Cb;
    double Fc;
    bool D; // Fc == F(Ca)? or F(Cb)
    TGoldenSearch(TFunc f, double a, double b)
        : F(f)
        , A(a)
        , B(b)
    {
        Ca = B - (B - A) * Phi1;
        Cb = A + (B - A) * Phi1;
        double FCa = F(Ca);
        double FCb = F(Cb);

        Check(FCa, FCb);
    }

    void Check(double FCa, double FCb) {
        if (IsNan(FCa))
            FCa = 1.e38;
        if (IsNan(FCb))
            FCb = 1.e38;

        if (FCa < FCb) {
            Fc = FCa;
            B = Cb;
            Cb = Ca;
            D = false;
        } else {
            Fc = FCb;
            A = Ca;
            Ca = Cb;
            D = true;
        }
    }

    void Step() {
        double FCa, FCb;
        if (D) {
            FCa = Fc;
            Cb = A + (B - A) * Phi1;
            FCb = F(Cb);
        } else {
            FCb = Fc;
            Ca = B - (B - A) * Phi1;
            FCa = F(Ca);
        }

        Check(FCa, FCb);
    }

    double Iterate(double err, int maxIter = 100) {
        for (int i = 0; i < maxIter && Error() > err; i++) {
            Step();
        }
        return Result();
    }

    double Error() {
        return 0.5 * (B - A);
    }

    double Result() {
        return 0.5 * (B + A);
    }
};

double GoldenSearch(std::function<double(double)> f, double a, double b, double err, int maxIter) {
    TGoldenSearch<std::function<double(double)>> gs(f, a, b);
    return gs.Iterate(err, maxIter);
}
