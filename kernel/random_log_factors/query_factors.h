#pragma once
#include <kernel/random_log_factors/factor_types.h>
#include <kernel/random_log_factors/factor_types.h_serialized.h>
#include <library/cpp/packedtypes/packedfloat.h>
#include <util/generic/cast.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/ysaveload.h>

namespace NRandomLogFactors {
    template <class EFactorType>
    class TRandomLogFactors {
    private:
        ui8 Version = 0;
        TVector<float> Factors;

    public:
        TRandomLogFactors();
        TRandomLogFactors(size_t factorsCount);
        float& operator[](EFactorType index);
        float at(EFactorType index) const;
        float GetValueOrDefault(EFactorType index, float defaultValue = 0.f) const;
        size_t Size() const;
        void Resize(size_t factorsCount);
        ui8 GetVersion() const;
        void SetVersion(ui8 version);
        void Load(IInputStream* in);
        void Save(IOutputStream* out) const;
    };

    template <class EFactorType>
    TRandomLogFactors<EFactorType>::TRandomLogFactors()
        : Factors(GetEnumItemsCount<EFactorType>())
    {
    }

    template <class EFactorType>
    TRandomLogFactors<EFactorType>::TRandomLogFactors(size_t factorsCount)
        : Factors(factorsCount)
    {
    }

    template <class EFactorType>
    float& TRandomLogFactors<EFactorType>::operator[](EFactorType index) {
        size_t factorInd = ToUnderlying(index);
        Y_VERIFY(factorInd < Factors.size(), "Provided factor index is greater than the storage size");
        return Factors[factorInd];
    }

    template <class EFactorType>
    float TRandomLogFactors<EFactorType>::at(EFactorType index) const {
        return Factors[ToUnderlying(index)];
    }

    template <class EFactorType>
    float TRandomLogFactors<EFactorType>::GetValueOrDefault(EFactorType index, float defaultValue) const {
        size_t factorInd = ToUnderlying(index);
        if (Y_UNLIKELY(factorInd >= Factors.size())) {
            return defaultValue;
        }
        return Factors[factorInd];
    }

    template <class EFactorType>
    size_t TRandomLogFactors<EFactorType>::Size() const {
        return Factors.size();
    }

    template <class EFactorType>
    void TRandomLogFactors<EFactorType>::Resize(size_t factorsCount) {
        Factors.resize(factorsCount, 0.f);
    }

    template <class EFactorType>
    ui8 TRandomLogFactors<EFactorType>::GetVersion() const {
        return Version;
    }

    template <class EFactorType>
    void TRandomLogFactors<EFactorType>::SetVersion(ui8 version) {
        Version = version;
    }

    template <class EFactorType>
    void TRandomLogFactors<EFactorType>::Load(IInputStream* in) {
        ::Load(in, Version);
        Y_ENSURE(Version == 0, "RandomLogQueryFactors Version differs from 0");
        ui32 factorsCount;
        ::Load(in, factorsCount);
        TString compressedFactors = in->ReadAll();
        Y_ENSURE(factorsCount == compressedFactors.size(), "Saved factors count does not equal loaded factors count");
        if (Factors) {
            Factors.clear();
        }
        Factors.reserve(factorsCount);
        for (size_t i = 0; i < factorsCount; ++i) {
            Factors.push_back(Frac2Float<ui8>(compressedFactors[i]));
        }
    }

    template <class EFactorType>
    void TRandomLogFactors<EFactorType>::Save(IOutputStream* out) const {
        ::Save(out, Version);
        ::Save(out, ui32(Factors.size()));
        TString compressedFactors;
        compressedFactors.reserve(Factors.size());
        for (float f : Factors) {
            compressedFactors += Float2Frac<ui8>(f);
        }
        out->Write(compressedFactors);
    }

    using TRandomLogQueryFactors = TRandomLogFactors<ERandomLogQueryFactor>;
    using TRandomLogWordBaseFactors = TRandomLogFactors<ERandomLogWordBaseFactor>;
}
