#pragma once

#include "matrix.h"
#include "options.h"
#include "pool.h"

#include <util/generic/algorithm.h>
#include <util/string/printf.h>
#include <util/generic/vector.h>

namespace NRegTree {

template <typename TFloatType>
struct TLinearModel {
    TVector<TFloatType> Coefficients;
    TFloatType Intercept;

    Y_SAVELOAD_DEFINE(Coefficients, Intercept);

    explicit TLinearModel(size_t featuresCount = 0)
        : Coefficients(featuresCount, TFloatType())
        , Intercept()
    {
    }

    template <typename T>
    TFloatType Prediction(const TVector<T>& features) const {
        return Intercept + std::inner_product(Coefficients.begin(), Coefficients.end(), features.begin(), TFloatType());
    }

    template <typename T>
    TFloatType ExtendedProduct(T iterator) const {
        TFloatType result = TFloatType();

        const TFloatType* coefficient = Coefficients.begin();
        for (; coefficient != Coefficients.end(); ++coefficient, ++iterator) {
            result += *coefficient * *iterator;
        }
        result += Intercept * *iterator;

        return result;
    }

    void InceptWeight(TFloatType weight) {
        Intercept *= weight;
        for (TFloatType* coefficient = Coefficients.begin(); coefficient != Coefficients.end(); ++coefficient) {
            *coefficient *= weight;
        }
    }

    void Reset() {
        std::fill(Coefficients.begin(), Coefficients.end(), TFloatType());
        Intercept = TFloatType();
    }

    void Print(IOutputStream& out, const TString& preffix = "") const {
        for (size_t i = 0; i < Coefficients.size(); ++i) {
            if (Coefficients[i]) {
                out << preffix << "attr" << (i + 1) << " * " << Sprintf("%.7lf +\n", Coefficients[i]);
            }
        }
        out << preffix << Intercept << "\n";
    }

    void Fix(const size_t featuresCount, const TSet<size_t>& ignoredFeatures) {
        TVector<TFloatType> fixedCoefficients(featuresCount);

        const TFloatType* coefficient = Coefficients.begin();
        for (size_t featureNumber = 0; featureNumber < featuresCount; ++featureNumber) {
            if (ignoredFeatures.contains(featureNumber)) {
                continue;
            }
            fixedCoefficients[featureNumber] = *coefficient;
            ++coefficient;
        }
        fixedCoefficients.swap(Coefficients);
    }
};

template <typename TFloatType>
class TLinearRegressionData {
private:
    size_t FeaturesCount;

    TFloatType SumSquaredGoals;

    TLinearizedTriangleOLSMatrix<TFloatType> OLSMatrix;
    TOLSVector<TFloatType> OLSVector;

    TLinearizedSquareOLSMatrix<TFloatType> DecompositionMatrix;
    TOLSVector<TFloatType> DecompositionTrace;
public:
    TLinearRegressionData(size_t featuresCount)
        : FeaturesCount(featuresCount)
        , SumSquaredGoals(TFloatType())
        , OLSMatrix(FeaturesCount)
        , OLSVector(FeaturesCount)
        , DecompositionMatrix(FeaturesCount)
        , DecompositionTrace(FeaturesCount)
    {
    }

    TLinearRegressionData<TFloatType>& operator += (const TLinearRegressionData<TFloatType>& other) {
        Y_ASSERT(FeaturesCount == other.FeaturesCount);

        SumSquaredGoals += other.SumSquaredGoals;

        OLSMatrix += other.OLSMatrix;
        OLSVector += other.OLSVector;

        return *this;
    }

    void AddTriangle(const TVector<TFloatType>& features, TFloatType goal, TFloatType weight) {
        SumSquaredGoals += goal * goal * weight;
        OLSMatrix.Add(features, weight);
        OLSVector.Add(features, goal, weight);
    }

    TFloatType SumSquaredErrors(const TLinearModel<TFloatType>& linearModel) const {
        TLinearizedSquareOLSMatrix<TFloatType> olsMatrix = OLSMatrix.RestoreFromTriangle();

        TFloatType matrixProduct = TFloatType();       // <X^T * X * a, a>
        const TFloatType* matrixElement = olsMatrix.begin();
        const TFloatType* solutionElement = linearModel.Coefficients.begin();
        for (; solutionElement != linearModel.Coefficients.end(); matrixElement += FeaturesCount + 1, ++solutionElement) {
            matrixProduct += *solutionElement * linearModel.ExtendedProduct(matrixElement);
        }
        matrixProduct += linearModel.Intercept * linearModel.ExtendedProduct(matrixElement);

        TFloatType sumSquaredErrors = matrixProduct - 2 * linearModel.ExtendedProduct(OLSVector.begin()) + SumSquaredGoals;
        return Max(TFloatType(), sumSquaredErrors);
    }

    void UpdateGoals(const TPool<TFloatType>& pool, const THashSet<size_t>& testInstances) {
        ::Fill(OLSVector.begin(), OLSVector.end(), TFloatType());

        SumSquaredGoals = TFloatType();
        for (const TInstance<TFloatType>* instance = pool.begin(); instance != pool.end(); ++instance) {
            if (testInstances.contains(instance - pool.begin())) {
                continue;
            }

            OLSVector.Add(instance->Features, instance->Goal, instance->Weight);
            SumSquaredGoals += instance->Goal * instance->Goal * instance->Weight;
        }
    }

    void Solve(TLinearModel<TFloatType>& linearModel,
               TFloatType regularizationParameter,
               TFloatType regularizationThreshold)
    {
        TLinearizedSquareOLSMatrix<TFloatType> olsMatrix = OLSMatrix.RestoreFromTriangle();

        TFloatType curRegularizationParameter = TFloatType();
        TFloatType nextRegularizationParameter = regularizationParameter;
        while (!CholeskyDecomposition(olsMatrix, curRegularizationParameter, regularizationThreshold)) {
            curRegularizationParameter = nextRegularizationParameter;
            nextRegularizationParameter *= 2;
        }

        linearModel.Reset();
        SolveLower(linearModel);

        const TFloatType* traceElement = DecompositionTrace.begin();
        TFloatType* coefficient = linearModel.Coefficients.begin();
        for (; coefficient != linearModel.Coefficients.end(); ++coefficient, ++traceElement) {
            *coefficient /= *traceElement;
        }
        linearModel.Intercept /= *traceElement;

        SolveUpper(linearModel);
    }
private:
    bool CholeskyDecomposition(const TLinearizedSquareOLSMatrix<TFloatType>& olsMatrix, TFloatType regularizationParameter, TFloatType regularizationThreshold) {
        TFloatType* traceElement = DecompositionTrace.begin();

        const TFloatType* olsMatrixDiagonalElement = olsMatrix.begin();
        TFloatType* decompositionMatrixElement = DecompositionMatrix.begin();
        TFloatType* decompositionMatrixSubDiagonalElement = decompositionMatrixElement + FeaturesCount + 1;

        for (; traceElement != DecompositionTrace.end(); ++traceElement) {
            TFloatType* decompositionMatrixRowElement = decompositionMatrixElement;

            *traceElement = *olsMatrixDiagonalElement + regularizationParameter;

            {
                TFloatType* currentTraceElement = DecompositionTrace.begin();
                while (currentTraceElement != traceElement) {
                    *traceElement -= *decompositionMatrixElement *
                                     *decompositionMatrixElement *
                                     *currentTraceElement;
                    ++currentTraceElement;
                    ++decompositionMatrixElement;
                }
            }

            if (*traceElement < regularizationThreshold) {
                return false;
            }

            *decompositionMatrixElement++ = 1;

            const TFloatType* olsMatrixElement = olsMatrixDiagonalElement + 1;

            TFloatType* nextDecompositionMatrixRow = decompositionMatrixRowElement + FeaturesCount + 1;
            TFloatType* columnDecompositionMatrixElement = decompositionMatrixSubDiagonalElement;

            while (nextDecompositionMatrixRow < DecompositionMatrix.end()) {
                TFloatType matrixElement = *olsMatrixElement++;

                TFloatType* leftDecompositionMatrixElement = decompositionMatrixRowElement;
                TFloatType* rightDecompositionMatrixElement = nextDecompositionMatrixRow;

                TFloatType* currentTraceElement = DecompositionTrace.begin();

                while (currentTraceElement != traceElement) {
                    matrixElement -= *leftDecompositionMatrixElement++ *
                                     *rightDecompositionMatrixElement++ *
                                     *currentTraceElement++;
                }

                TFloatType value = matrixElement / *traceElement;

                *decompositionMatrixElement++ = value;
                *columnDecompositionMatrixElement = value;

                nextDecompositionMatrixRow += FeaturesCount + 1;
                columnDecompositionMatrixElement += FeaturesCount + 1;
            }

            olsMatrixDiagonalElement += FeaturesCount + 2;
            decompositionMatrixSubDiagonalElement += FeaturesCount + 2;
        }

        return true;
    }

    void SolveUpper(TLinearModel<TFloatType>& solution) {
        TFloatType* coefficient = solution.Coefficients.end() - 1;
        TFloatType* matrixRowStart = DecompositionMatrix.end() - FeaturesCount - 2;
        TFloatType* matrixRowEnd = DecompositionMatrix.end() - FeaturesCount - 2;

        while (coefficient >= solution.Coefficients.begin()) {
            *coefficient -= std::inner_product(matrixRowStart, matrixRowEnd, coefficient + 1, TFloatType()) + *matrixRowEnd * solution.Intercept;

            --coefficient;
            matrixRowStart -= FeaturesCount + 2;
            matrixRowEnd -= FeaturesCount + 1;
        }
    }

    void SolveLower(TLinearModel<TFloatType>& solution) {
        TFloatType* coefficient = solution.Coefficients.begin();
        TFloatType* matrixRowStart = DecompositionMatrix.begin();
        TFloatType* matrixRowEnd = DecompositionMatrix.begin();
        TFloatType* target = OLSVector.begin();

        while (coefficient != solution.Coefficients.end()) {
            *coefficient++ += *target++ - std::inner_product(matrixRowStart, matrixRowEnd, solution.Coefficients.begin(), TFloatType());

            matrixRowStart += FeaturesCount + 1;
            matrixRowEnd += FeaturesCount + 2;
        }

        solution.Intercept += *target - std::inner_product(matrixRowStart, matrixRowEnd, solution.Coefficients.begin(), TFloatType());
    }
};

}
