#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/gradient_optimize/grad.h>
#include <library/cpp/gradient_optimize/fista.h>

template <class TTData>
struct TNumericGradient {
    TTData& Data;

    TNumericGradient(TTData& data)
        : Data(data)
    {
    }

    TVectorType<double> operator()(const TVectorType<double>& x0) {
        TVectorType<double> g(x0.ysize());
        for (int i = 0; i < x0.ysize(); i++) {
            TVectorType<double> x = x0;
            x[i] += 1e-10; //FIXME: auto scale!
            double y2 = Data(x);
            x[i] -= 2e-10;
            double y1 = Data(x);
            //            Cout << i << ": " << y2 << " - " << y1 << " / 2e-10 = " << (y2 - y1) / 2e-10 << Endl;
            g[i] = (y2 - y1) / 2e-10;
        }

        //Cout << "g: " << g << Endl;
        return g;
    }
};

template <class TTData>
TVectorType<double> NumericGradient(TTData& data, const TVectorType<double>& x0) {
    TNumericGradient<TTData> grad(data);
    return grad(x0);
}

struct TMyFunc {
    template <class T>
    T operator()(const TVectorType<T>& x) {
        T y = 0.;
        T z = 0.;
        for (int i = 0; i < x.ysize() / 2; i++) {
            y += exp(logistic(x[i]) * logistic(x[x.size() - i - 1]));
            z += Sqr(x[i]) + Sqr(x[x.size() - i - 1]);
        }
        return log(y) / sqrt(z);
    }
};

class TVariationGradientTest: public TTestBase {
    UNIT_TEST_SUITE(TVariationGradientTest);
    UNIT_TEST(Test);
    UNIT_TEST_SUITE_END();

public:
    void Test() {
        TMyFunc data;
        TVectorType<double> x(6);
        for (int i = 0; i < x.ysize(); i++) {
            x[i] = (i + 1);
        }
        //        Cerr << NumericGradient(data, x) << Endl;
        //        Cerr << VariationGradient(data, x) << Endl;
        auto err = NumericGradient(data, x) - VariationGradient(data, x);
        UNIT_ASSERT_DOUBLES_EQUAL(sqrt(err % err), 0., 1e-5);
    }
};
UNIT_TEST_SUITE_REGISTRATION(TVariationGradientTest);

struct TMyData {
    template <class T>
    T operator()(const TVectorType<T>& x) {
        T e = 0.;
        for (int i = 0; i < 4; i++) {
            T b = 4 - i;
            for (int j = 0; j < 3; j++) {
                b -= x[j] / (i + j + 1);
            }
            e += Sqr(b);
        }
        return e;
    }
};

struct TMyDataL2 {
    TMyData F;

    template <class T>
    T operator()(const TVectorType<T>& x) {
        return F(x) + x % x;
    }
};

struct TMyDataL1 {
    TMyData F;
    TL1Regularization H = TL1Regularization(0.2);

    template <class T>
    T operator()(const TVectorType<T>& x) {
        return F(x) + H(x);
    }
};

class TConjugateGradientDescendTest: public TTestBase {
    UNIT_TEST_SUITE(TConjugateGradientDescendTest);
    UNIT_TEST(TestL2);
    UNIT_TEST(TestL1);
    UNIT_TEST_SUITE_END();

public:
    void TestL2() {
        TMyDataL2 f;
        TVectorType<double> x1(3, 1.);
        TVectorType<double> x2(3, 1.);

        TConjugateGradientDescend<decltype(f)> cgd(f, x1);
        for (int i = 0; i < 10; i++) {
            auto err = f(x1);
            cgd.Step();
            UNIT_ASSERT(f(x1) <= err); // cgd is a descend
        }

        TSimpleFista<decltype(f)> fista(x2, f);
        fista.Step(10);
        UNIT_ASSERT(f(x1) < f(x2)); // fista converges slower at the beginning

        fista.Step(50);
        cgd.Step(10);
        UNIT_ASSERT_DOUBLES_EQUAL(sqrt((x1 - x2) % (x1 - x2)), 0., 1e-2); // approximately the same solution

        TVectorType<double> x3(3, 2.);
        TConjugateGradientDescend<decltype(f)>(f, x3).Step(20);
        UNIT_ASSERT_DOUBLES_EQUAL(sqrt((x1 - x3) % (x1 - x3)), 0., 1e-2); // initial point doesn't matter
    }

    void TestL1() {
        TMyDataL1 f;
        TMyData g;

        TVectorType<double> x1(3, 1.);
        TVectorType<double> x2(3, 1.);
        TConjugateGradientDescendOptions opts;
        //opts.Verbose = true;
        TConjugateGradientDescend<decltype(f)> cgd(f, x1, opts);
        TL1Fista<decltype(g)> fista(x2, g, f.H);

        cgd.Step(100);
        fista.Step(100);
        UNIT_ASSERT(f(x1) > f(x2)); // fista handles L1 problems better

        //Cerr << "grad: " << VariationGradient(f, x1) << Endl;
        //Cerr << "f(" << x1 << ") = " << f(x1) << " vs " << "f(" << x2 << ") = " << f(x2) << Endl;
        //UNIT_ASSERT_DOUBLES_EQUAL(sqrt((x1 - x2) % (x1 - x2)), 0., 1e-2); // FIXME: Fails! :)
    }
};
UNIT_TEST_SUITE_REGISTRATION(TConjugateGradientDescendTest);
