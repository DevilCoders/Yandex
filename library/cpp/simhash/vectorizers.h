#pragma once

#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/generic/algorithm.h>
#include <util/generic/utility.h>
#include <util/generic/yexception.h>
#include <util/generic/ymath.h>
#include <util/system/defaults.h>
#include <util/system/yassert.h>

#include <math.h>

//template<class TInValueType>
//class TDummyVectorizer: private TNonCopyable {
//public:
//    typedef TInValueType TInValue;
//    typedef i32 TOutValue;
//    typedef TVector<TInValue> TIn;
//    typedef TVector<TOutValue> TOut;

//    static ui32 GetVersion() { return 1; }
//    static TString GetDescription() { return "simple modulo based vectorizer"; }

//public:
//    TDummyVectorizer(
//        TIn * Input,
//        TOut * Output)
//        : input(Input)
//        , output(Output)
//    {
//        Y_VERIFY((input != NULL), "input == NULL");
//        Y_VERIFY((output != NULL), "output == NULL");
//    }

//public:
//    void Vectorize() {
//        const size_t inputSize = input->size();
//        const size_t outputSize = output->size();
//        for (size_t i = 0; i < inputSize; ++i) {
//            output->operator[](input->operator[](i) % outputSize) += 1;
//        }
//    }

//private:
//    TIn * input;
//    TOut * output;
//};

//template<class TInValueType>
//class TDummyVectorizer2: private TNonCopyable {
//public:
//    typedef TInValueType TInValue;
//    typedef i32 TOutValue;
//    typedef TVector<TInValue> TIn;
//    typedef TVector<TOutValue> TOut;

//    static ui32 GetVersion() { return 2; }
//    static TString GetDescription() { return "simple modulo based vectorizer with negative output values"; }

//private:
//    template<ui32 A, ui32 B>
//    struct TSimpleHashFunction {
//        static ui32 Calc(TInValue value, ui32 M) {
//            return (A * value + B) % M;
//        }
//    };

//public:
//    TDummyVectorizer2(
//        TIn * Input,
//        TOut * Output)
//        : input(Input)
//        , output(Output)
//    {
//        Y_VERIFY((input != NULL), "input == NULL");
//        Y_VERIFY((output != NULL), "output == NULL");
//    }

//public:
//    void Vectorize() {
//        const ui32 inputSize = (ui32)input->size();
//        const ui32 outputSize = (ui32)output->size();

//        for (ui32 i = 0; i < inputSize; ++i) {
//            TInValue value = input->operator[](i);

//            ui32 index1 = TSimpleHashFunction<3571, 1873>::Calc(value, outputSize);
//            ui32 index2 = TSimpleHashFunction<1567, 3559>::Calc(value, outputSize);

//            if (index1 == index2) {
//                index1 = (index1 + outputSize - 1) % outputSize;
//                index2 = (index2 + 1) % outputSize;
//            }

//            ui32 addVal = TSimpleHashFunction<2473, 911>::Calc(value, 11);
//            ui32 subVal = 10 - addVal;
//            output->operator[](index1) += addVal;
//            output->operator[](index2) -= subVal;
//        }
//    }

//private:
//    TIn * input;
//    TOut * output;
//};

//template<class TInValueType>
//class TDummyVectorizer3: private TNonCopyable {
//public:
//    typedef TInValueType TInValue;
//    typedef i32 TOutValue;
//    typedef TVector<TInValue> TIn;
//    typedef TVector<TOutValue> TOut;

//    static ui32 GetVersion() { return 3; }
//    static TString GetDescription() { return "simple modulo based vectorizer with negative output values and megafeature"; }

//private:
//    template<ui32 A, ui32 B>
//    struct TSimpleHashFunction {
//        static ui32 Calc(TInValue value, ui32 M) {
//            return (A * value + B) % M;
//        }
//    };

//public:
//    TDummyVectorizer3(
//        TIn * Input,
//        TOut * Output)
//        : input(Input)
//        , output(Output)
//    {
//        Y_VERIFY((input != NULL), "input == NULL");
//        Y_VERIFY((output != NULL), "output == NULL");
//    }

//public:
//    void Vectorize() {
//        /*
//        1051 1061 1063 1069 1087 1091 1093 1097 1103 1109 1811 1823 1831
//        1847 1861 1867 1871 1873 1877 1879 1889 1901 1907 3181 3187 3191
//        3203 3209 3217 3221 3229 3251 3253 3257 3259 4651 4657 4663 4673
//        4679 4691 4703 4721 4723 4729 4733 4751 4759 4783 5059 5179 5737
//        */
//        const ui32 inputSize = (ui32)input->size();
//        const ui32 outputSize = (ui32)output->size();
//        if (outputSize < 3) {
//            ythrow yexception() << "Vector lengths < 3 not supported";
//        }
//        for (ui32 i = 0; i < inputSize; ++i) {
//            TInValue value = input->operator[](i);

//            ui32 index1 = TSimpleHashFunction<3571, 1873>::Calc(value, outputSize);
//            ui32 index2 = TSimpleHashFunction<1567, 3559>::Calc(value, outputSize - 1);
//            ui32 index3 = TSimpleHashFunction<3229, 1103>::Calc(value, outputSize - 2);

//            if (index3 >= index2) {
//                ++index3;
//            }
//            if (index3 >= index1) {
//                ++index3;
//            }
//            if (index2 >= index1) {
//                ++index2;
//            }

//            //Cout << index1 << " " << index2 << " " << index3 << " ";
//            Y_VERIFY(((index1 != index2) && (index1 != index3) && (index2 != index3)), "Index match: your program is wrong");

//            double angleXY = 2.0 * M_PI * TSimpleHashFunction<3253, 1051>::Calc(value, 360) / 360.0;
//            double angleZ = acos(2.0 * TSimpleHashFunction<4783, 5737>::Calc(value, 10001) / 10000.0 - 1);

//            double addX = sin(angleZ) * cos(angleXY);
//            double addY = sin(angleZ) * sin(angleXY);
//            double addZ = cos(angleZ);

//            double valX = output->operator[](index1);
//            double valY = output->operator[](index2);
//            double valZ = output->operator[](index3);
//            double len0 = sqrt(valX * valX + valY * valY + valZ * valZ);

//            valX += addX * 10;
//            valY += addY * 10;
//            valZ += addZ * 10;
//            double lent = sqrt(valX * valX + valY * valY + valZ * valZ);
//            double koef = GetKoef(len0, lent);

//            //Cout << addX << " " << addY << " " << addZ << Endl;

//            output->operator[](index1) = i32(koef * valX);
//            output->operator[](index2) = i32(koef * valY);
//            output->operator[](index3) = i32(koef * valZ);
//        }
//    }

//private:
//    double Potential(double x) {
//        if (std::abs(x - 100.0) <= 0.000001) {
//            return 100000.0;
//        }
//        return -100.0 / (x - 100.0);
//    }

//    double PotentialDerivative(double x) {
//        if (std::abs(x - 100.0) <= 0.001) {
//            return 100000.0;
//        }
//        return 100.0 / ((x - 100.0) * (x - 100.0));
//    }

//    double GetKoef(double x0, double xt) {
//        if (x0 < 0.001) {
//            return 1.0;
//        }
//        if (xt < 0.001) {
//            return 1.0;
//        }
//        double xtmx02 = (xt - x0) * (xt - x0);
//        if (xtmx02 < 0.001) {
//            if (xt >= 100.0) {
//                return 99.5 / xt;
//            }
//            return 1.0;
//        }
//        double potential = Potential(x0);
//        double potentialDerivative = PotentialDerivative(x0);
//        double ytmy0 = potentialDerivative * xt + potential / (potentialDerivative * x0) - potential;
//        double x1 = x0;
//        if (xt > x0) {
//            x1 += xtmx02 / sqrt(xtmx02 + ytmy0 * ytmy0);
//        } else {
//            x1 -= xtmx02 / sqrt(xtmx02 + ytmy0 * ytmy0);
//        }
//        if (x1 >= 100.0) {
//            x1 = 99.5;
//        }

//        return x1 / xt;
//    }

//private:
//    TIn * input;
//    TOut * output;
//};

template <class TInValueType>
class TDummyVectorizer4: private TNonCopyable {
public:
    typedef TInValueType TInValue;
    typedef i32 TOutValue;
    typedef TVector<TInValue> TIn;
    typedef TVector<TOutValue> TOut;

    static ui32 GetVersion() {
        return 4;
    }
    static TString GetDescription() {
        return "simple modulo based vectorizer with negative output values and workaround for vk.com";
    }

private:
    template <ui32 A, ui32 B>
    struct TSimpleHashFunction {
        static ui32 Calc(const TInValue value, ui32 M) {
            return (A * value + B) % M;
        }
    };

public:
    TDummyVectorizer4(
        TIn* Input,
        TOut* Output)
        : input(Input)
        , output(Output)
        , stabilizedInput()
    {
        Y_VERIFY((input != nullptr), "input == NULL");
        Y_VERIFY((output != nullptr), "output == NULL");
    }

public:
    ui32 Vectorize() {
        if (input->size() == 0) {
            return 0;
        }

        PreprocessInput();

        const ui32 inputSize = (ui32)stabilizedInput.size();
        const ui32 outputSize = (ui32)output->size();

        for (ui32 i = 0; i < inputSize; ++i) {
            const TInValue value = stabilizedInput[i].first;
            const ui32 count = stabilizedInput[i].second;

            ui32 index1 = TSimpleHashFunction<3571, 1873>::Calc(value, outputSize);
            ui32 index2 = TSimpleHashFunction<1567, 3559>::Calc(value, outputSize);
            if (index1 == index2) {
                index1 = (index1 + outputSize - 1) % outputSize;
                index2 = (index2 + 1) % outputSize;
            }

            ui32 addVal = TSimpleHashFunction<2473, 911>::Calc(value, 11);
            ui32 subVal = 10 - addVal;
            output->operator[](index1) += addVal * count;
            output->operator[](index2) -= subVal * count;
        }

        return inputSize;
    }

private:
    static bool CompareByCount(
        const std::pair<TInValue, ui32>& left,
        const std::pair<TInValue, ui32>& right) {
        return left.second < right.second;
    }

    void PreprocessInput() {
        TIn& input2 = *this->input;
        const size_t inputSize = input2.size();

        TVector<std::pair<TInValue, ui32>>& counts = stabilizedInput;
        counts.clear();

        Sort(input2.begin(), input2.end());

        if (counts.capacity() < inputSize) {
            counts.reserve(inputSize);
        }

        ui32 count = 1;
        for (size_t i = 0; i < inputSize; ++i) {
            if ((i + 1 == inputSize) || (input2[i] != input2[i + 1])) {
                counts.push_back(std::make_pair(input2[i], count));
                count = 1;
            } else {
                ++count;
            }
        }

        Sort(counts.begin(), counts.end(), CompareByCount);
        ui32 order = 1;

        const size_t countsSize = counts.size();

        for (size_t i = 0; i < countsSize; ++i) {
            ui32 currentOrder = order;
            if ((i + 1 < countsSize) && (counts[i].second != counts[i + 1].second)) {
                ++order;
            }
            counts[i].second = currentOrder;
        }
    }

private:
    TIn* input;
    TOut* output;
    TVector<std::pair<TInValue, ui32>> stabilizedInput;
};
