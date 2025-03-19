#pragma once

#include "options.h"
#include "pool.h"

#include <util/generic/algorithm.h>
#include <util/string/printf.h>
#include <util/generic/vector.h>

#ifdef _sse2_
    #include <emmintrin.h>
    #include <xmmintrin.h>
#endif

namespace NRegTree {

#ifdef _sse2_
    template <typename TFloatType>
    inline void AddFeaturesProduct(const TFloatType weight,
                                   const TVector<TFloatType>& features,
                                   TVector<TFloatType>& linearizedTriangleMatrix);

    template <>
    inline void AddFeaturesProduct(const float weight,
                                   const TVector<float>& features,
                                   TVector<float>& linearizedTriangleMatrix)
    {
        const float* leftFeature = features.begin();
        float* matrixElement = linearizedTriangleMatrix.begin();

        size_t unaligned = features.size() & 0x3;

        for (; leftFeature != features.end(); ++leftFeature, ++matrixElement) {
            float weightedFeature = weight * *leftFeature;
            const float* rightFeature = leftFeature;
            __m128 wf = { weightedFeature, weightedFeature, weightedFeature, weightedFeature };
            for (size_t i = 0; i < unaligned; ++i, ++rightFeature, ++matrixElement) {
                *matrixElement += weightedFeature * *rightFeature;
            }
            unaligned = (unaligned + 3) & 0x3;
            for (; rightFeature != features.end(); rightFeature += 4, matrixElement += 4) {
                __m128 rf = _mm_loadu_ps(rightFeature);
                __m128 matrixRow = _mm_loadu_ps(matrixElement);
                __m128 rowAdd = _mm_mul_ps(rf, wf);
                _mm_storeu_ps(matrixElement, _mm_add_ps(rowAdd, matrixRow));
            }
            *matrixElement += weightedFeature;
        }
        linearizedTriangleMatrix.back() += weight;
    }

    template <>
    inline void AddFeaturesProduct(const double weight,
                                   const TVector<double>& features,
                                   TVector<double>& linearizedTriangleMatrix)
    {
        const double* leftFeature = features.begin();
        double* matrixElement = linearizedTriangleMatrix.begin();

        size_t unaligned = features.size() & 0x1;

        for (; leftFeature != features.end(); ++leftFeature, ++matrixElement) {
            double weightedFeature = weight * *leftFeature;
            const double* rightFeature = leftFeature;
            __m128d wf = { weightedFeature, weightedFeature };
            for (size_t i = 0; i < unaligned; ++i, ++rightFeature, ++matrixElement) {
                *matrixElement += weightedFeature * *rightFeature;
            }
            unaligned = (unaligned + 1) & 0x1;
            for (; rightFeature != features.end(); rightFeature += 2, matrixElement += 2) {
                __m128d rf = _mm_loadu_pd(rightFeature);
                __m128d matrixRow = _mm_loadu_pd(matrixElement);
                __m128d rowAdd = _mm_mul_pd(rf, wf);
                _mm_storeu_pd(matrixElement, _mm_add_pd(rowAdd, matrixRow));
            }
            *matrixElement += weightedFeature;
        }
        linearizedTriangleMatrix.back() += weight;
    }
#else
    template <typename TFloatType>
    static inline void AddFeaturesProduct(const TFloatType weight, const TVector<TFloatType>& features, TVector<TFloatType>& linearizedTriangleMatrix) {
        const TFloatType* leftFeature = features.begin();
        TFloatType* matrixElement = linearizedTriangleMatrix.begin();
        for (; leftFeature != features.end(); ++leftFeature, ++matrixElement) {
            TFloatType weightedFeature = weight * *leftFeature;
            const TFloatType* rightFeature = leftFeature;
            for (; rightFeature != features.end(); ++rightFeature, ++matrixElement) {
                *matrixElement += weightedFeature * *rightFeature;
            }
            *matrixElement += weightedFeature;
        }
        linearizedTriangleMatrix.back() += weight;
    }
#endif

template <typename TFloatType>
class TLinearizedSquareOLSMatrix : public TVector<TFloatType> {
private:
    typedef TVector<TFloatType> TBase;

    size_t Size;
public:
    TLinearizedSquareOLSMatrix(size_t featuresCount)
        : TBase((featuresCount + 1) * (featuresCount + 1))
        , Size(featuresCount + 1)
    {
    }
};

template <typename TFloatType>
class TLinearizedTriangleOLSMatrix : public TVector<TFloatType> {
private:
    typedef TVector<TFloatType> TBase;

    size_t Size;
public:
    TLinearizedTriangleOLSMatrix(size_t featuresCount)
        : TBase((featuresCount + 1) * (featuresCount + 2) / 2)
        , Size(featuresCount + 1)
    {
    }

    void Add(const TVector<TFloatType>& features, const TFloatType weight) {
        AddFeaturesProduct(weight, features, *this);
    }

    TLinearizedSquareOLSMatrix<TFloatType> RestoreFromTriangle() const {
        TLinearizedSquareOLSMatrix<TFloatType> result(Size - 1);

        const TFloatType* myElement = this->begin();
        for (TFloatType* diagonalElement = result.begin(); diagonalElement < result.end(); diagonalElement += Size + 1) {
            TFloatType* rowElement = diagonalElement;
            TFloatType* columnElement = diagonalElement;
            while (columnElement < result.end()) {
                *columnElement = *rowElement = *myElement;
                ++myElement;
                ++rowElement;
                columnElement += Size;
            }
        }
        return result;
    }

    TLinearizedTriangleOLSMatrix<TFloatType>& operator += (const TLinearizedTriangleOLSMatrix<TFloatType>& other) {
        TFloatType* element = this->begin();
        const TFloatType* otherElement = other.begin();
        for (; element != this->end(); ++element, ++otherElement) {
            *element += *otherElement;
        }
        return *this;
    }
};

template <typename TFloatType>
class TOLSVector : public TVector<TFloatType> {
private:
    typedef TVector<TFloatType> TBase;
public:
    TOLSVector(size_t featuresCount)
        : TBase(featuresCount + 1)
    {
    }

    void Add(const TVector<TFloatType>& features, TFloatType goal, TFloatType weight) {
        TFloatType weightedGoal = goal * weight;
        TFloatType* vectorElement = this->begin();
        for (const TFloatType* feature = features.begin(); feature != features.end(); ++feature, ++vectorElement) {
            *vectorElement += *feature * weightedGoal;
        }
        *vectorElement += weightedGoal;
    }

    TOLSVector<TFloatType>& operator += (const TOLSVector<TFloatType>& other) {
        TFloatType* element = this->begin();
        const TFloatType* otherElement = other.begin();
        for (; element != this->end(); ++element, ++otherElement) {
            *element += *otherElement;
        }
        return *this;
    }
};


}
