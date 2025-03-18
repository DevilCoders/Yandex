#pragma once

#include <util/draft/matrix.h>

#include <util/generic/yexception.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>

#include <util/system/defaults.h>
#include <util/system/yassert.h>

class TRandomVectors {
public:
    typedef i32 TRandomValue;

public:
    TRandomVectors(ui32 Length, ui32 Count)
        : vectors((int)Count, (int)Length)
        , covariations()
        , expectations()
    {
    }

    TRandomVectors(i32* DataBuffer, float* StatBuffer)
        : vectors()
        , covariations()
        , expectations()
    {
        Init(DataBuffer, StatBuffer);
    }

public:
    ui32 Length() const {
        return (ui32)vectors.Width();
    }

    ui32 Count() const {
        return (ui32)vectors.Height();
    }

    void Resize(ui32 Length, ui32 Count) {
        vectors.ReDim((int)Count, (int)Length);
        covariations.Destroy();
        expectations.Destroy();
    }

    void ResetStatData() {
        covariations.Reset(new TMatrix<float>(Count(), Count()));
        expectations.Reset(new TMatrix<float>(1, Length()));
    }

    void Init(ui32 VectorLength, ui32 VectorCount,
              i32* VectorsBuffer,
              float* CovariationsBuffer,
              float* ExpectationsBuffer) {
        Y_VERIFY((VectorsBuffer != nullptr), "VectorsBuffer == NULL");

        vectors.~TMatrix();
        covariations.Destroy();
        expectations.Destroy();

        new (&vectors) TMatrix<i32>((int)VectorCount, (int)VectorLength, VectorsBuffer);

        if (CovariationsBuffer != nullptr) {
            covariations.Reset(new TMatrix<float>((int)VectorCount, (int)VectorCount, CovariationsBuffer));
        }
        if (ExpectationsBuffer != nullptr) {
            expectations.Reset(new TMatrix<float>(1, (int)VectorLength, ExpectationsBuffer));
        }
    }

    void Init(i32* DataBuffer, float* StatBuffer) {
        ui32 count = (ui32) * (DataBuffer + 0);
        ui32 length = (ui32) * (DataBuffer + 1);
        i32* vectorsBuffer = DataBuffer + 2;
        if (StatBuffer != nullptr) {
            float* covariationsBuffer = StatBuffer;
            float* expectationsBuffer = StatBuffer + count * count;

            Init(length, count, vectorsBuffer, covariationsBuffer, expectationsBuffer);
        } else {
            Init(length, count, vectorsBuffer, nullptr, nullptr);
        }
    }

    bool HasCovariationMatrix() const {
        return covariations.Get() != nullptr;
    }
    bool HasExpectationVector() const {
        return expectations.Get() != nullptr;
    }

    const TMatrix<i32>& Vectors() const {
        return vectors;
    }
    TMatrix<i32>& Vectors() {
        return vectors;
    }

    const TMatrix<float>& Cov() const {
        if (!HasCovariationMatrix()) {
            ythrow yexception() << "No covariations";
        }
        return *covariations.Get();
    }
    TMatrix<float>& Cov() {
        if (!HasCovariationMatrix()) {
            ythrow yexception() << "No covariations";
        }
        return *covariations.Get();
    }

    const TMatrix<float>& Exp() const {
        if (!HasExpectationVector()) {
            ythrow yexception() << "No expectations";
        }
        return *expectations.Get();
    }
    TMatrix<float>& Exp() {
        if (!HasExpectationVector()) {
            ythrow yexception() << "No expectations";
        }
        return *expectations.Get();
    }

    void Fill(i32 VectorValue = 0, float StatValue = 0.0f) {
        vectors.Fill(VectorValue);

        if (HasCovariationMatrix()) {
            covariations->Fill(StatValue);
        }
        if (HasExpectationVector()) {
            expectations->Fill(StatValue);
        }
    }

private:
    TMatrix<i32> vectors;
    THolder<TMatrix<float>> covariations;
    THolder<TMatrix<float>> expectations;
};

struct TRandomVectorsDescription {
    ui32 Version;
    ui32 Length;
    ui32 Count;

    TRandomVectorsDescription(ui32 Vrsn, ui32 Lngth, ui32 Cnt)
        : Version(Vrsn)
        , Length(Lngth)
        , Count(Cnt)
    {
    }
};

class TRandomVectorsFactory {
public:
    TRandomVectorsFactory();

public:
    const TRandomVectors& GetRandomVectors(ui32 Version) const;
    void GetAvailableVersions(TVector<TRandomVectorsDescription>* Versions) const;

private:
    THashMap<ui32, TSimpleSharedPtr<TRandomVectors>> vectors;
};

TRandomVectorsFactory& RandomVectorsFactory();
