#pragma once

#include <kernel/matrixnet/mn_sse.h>
#include <kernel/matrixnet/model_info.h>
#include <kernel/matrixnet/relev_calcer.h>

#include <library/cpp/json/writer/json_value.h>
#include <util/generic/vector.h>
#include <util/generic/ptr.h>


namespace NMatrixnet {
    template<typename T>
    struct TRenorm {
        static_assert(std::is_arithmetic_v<T>);
    public:
        TRenorm(T mult, T add)
            : Mult(mult)
            , Add(add)
        {
        }

        TRenorm()
            : TRenorm(T(1), T(0))
        {
        }

    public:
        T Apply(T relevance) const {
            return relevance * Mult + Add;
        }
        void SetMult(const T mult) {
            Mult = mult;
        }
        void SetAdd(const T add) {
            Add = add;
        }
        T GetMult() const {
            return Mult;
        }
        T GetAdd() const {
            return Add;
        }

        static TRenorm<T> Sum(const TRenorm<T>& first, const TRenorm<T>& second) {
            return {first.Mult + second.Mult, first.Add + second.Add};
        }
        static TRenorm<T> Compose(const TRenorm<T>& after, const TRenorm<T>& before) {
            // result.Apply(x) = after.Apply(before.Apply(x))
            return {after.Mult * before.Mult, after.Add + before.Add * after.Mult};
        }

        bool operator==(const TRenorm& other) const {
            return other.Mult == Mult && other.Add == Add;
        }

    private:
        T Mult;
        T Add;
    };

    template<typename T>
    IOutputStream& operator<<(IOutputStream& out, const TRenorm<T>& value) {
        return out << "(x * " << value.GetMult() << " + " << value.GetAdd() << ")";
    }

    using TBundleRenorm = TRenorm<double>;

    template <class T>
    struct TBundleElement {
    public:
        T Matrixnet;
        TBundleRenorm Renorm;

    public:
        TBundleElement(const T& matrixnet, TBundleRenorm renorm = {})
            : Matrixnet(matrixnet)
            , Renorm(renorm)
        {
        }

        bool operator==(const TBundleElement<T>& other) const {
            return Matrixnet == other.Matrixnet && Renorm == other.Renorm;
        }
    };

    struct TBundleDescription {
        TModelInfo Info;
        TVector<TBundleElement<TString>> Elements;
    };

    using TWeightedMatrixnet = TBundleElement<TMnSsePtr>;
    using TRankModelVector = TVector<TWeightedMatrixnet>;

    class TBundle : public IRelevCalcer {
    public:
        TBundle(const TRankModelVector& matrixnets, const TModelInfo& info);

        size_t GetNumFeats() const override;

        double DoCalcRelev(const float* features) const override;
        void DoCalcRelevs(const float* const* features, double* relevs, const size_t numDocs) const override;
        void DoSlicedCalcRelevs(const TFactorStorage* const* features, double* relevs, size_t numDocs) const override;

        const TModelInfo* GetInfo() const override;

        TString GetId() const override;
        void UsedFactors(TSet<ui32>& factors) const override;
        void UsedFactors(TSet<NFactorSlices::TFullFactorIndex>& factors) const override;
        static TBundleDescription ParseBundle(const NJson::TJsonValue::TMapType& sum, const NJson::TJsonValue::TMapType& bundleInfo);
    public:
        const TRankModelVector Matrixnets;
    private:
        TModelInfo ModelInfo;
        void Verify();
    };

    using TBundlePtr = TAtomicSharedPtr<TBundle>;

} // NMatrixnet

TString ToString(const NMatrixnet::TRankModelVector& v);
