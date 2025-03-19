#pragma once

#include <kernel/factor_slices/factor_borders.h>
#include <kernel/factor_storage/factor_storage.h>
#include <kernel/matrixnet/mn_sse.h>
#include <kernel/matrixnet/model_info.h>
#include <kernel/matrixnet/relev_calcer.h>
#include <kernel/matrixnet/sliced_util.h>

#include <catboost/libs/model/model.h>

#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/threading/thread_local/thread_local.h>

#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/string/type.h>


namespace NCatboostCalcer {

    // TODO support multiclass
    // TODO postprocess RAW predictions to class labels or probabilities

    namespace NDetail {
        struct TCalcContext {
            TVector<float> Holder;
            TVector<TConstArrayRef<int>> EmptyCatFeatures;
            TVector<TConstArrayRef<float>> FinalFeatures;
        };
    }

    class TCatboostCalcer : public NMatrixnet::IRelevCalcer {
    private:
        static NThreading::TThreadLocalValue<NDetail::TCalcContext> CalcContext;

        TFullModel Model_;
        TVector<NMatrixnet::TFeatureSlice> Slices_;
        TVector<TString> SliceNames_;

    public:
        TCatboostCalcer() = default;

        explicit TCatboostCalcer(TFullModel&& model) {
            Model_ = std::move(model);
            SetSlicesFromInfo();
        }

        explicit TCatboostCalcer(const TFullModel& model) {
            Model_ = model;
            SetSlicesFromInfo();
        }

        size_t GetNumFeats() const override {
            return Model_.GetNumFloatFeatures();
        }

        using NMatrixnet::IRelevCalcer::CalcRelev;
        using NMatrixnet::IRelevCalcer::CalcRelevs;

        size_t GetTreeCount() const {
            return Model_.GetTreeCount();
        }

        void SetEvaluatorType(EFormulaEvaluatorType modelEvaluatorType) {
            Model_.SetEvaluatorType(modelEvaluatorType);
        }

        double DoCalcRelev(const float* features) const override {
            double result;
            DoCalcRelevs(&features, &result, 1);
            return result;
        }

        void DoCalcRelevs(const TVector<TConstArrayRef<float>>& floatFeaturesVec, double* resultRelev, const size_t numDocs) const {
            NDetail::TCalcContext& context = CalcContext.GetRef();
            context.EmptyCatFeatures.resize(numDocs);
            Model_.Calc(floatFeaturesVec, context.EmptyCatFeatures, MakeArrayRef(resultRelev, numDocs));
        }

        void DoCalcRelevs(const float* const* docsFeatures, double* resultRelev, const size_t numDocs) const override {
            auto numFloatFeatures = Model_.GetNumFloatFeatures();
            NDetail::TCalcContext& context = CalcContext.GetRef();
            context.FinalFeatures.resize(numDocs);
            for (size_t docId = 0; docId < numDocs; ++docId) {
                context.FinalFeatures[docId] = MakeArrayRef(docsFeatures[docId], numFloatFeatures);
            }
            DoCalcRelevs(context.FinalFeatures, resultRelev, numDocs);
        }

        const TVector<NMatrixnet::TFeatureSlice>& GetSlices() const {
            return Slices_;
        }
        const TVector<TString>& GetSliceNames() const {
            return SliceNames_;
        }

        template <class TFactorStorageType>
        void GetSlicedFeatures(
            const TFactorStorageType features[],
            const size_t numDocs,
            TVector<float>* holder,
            TVector<TConstArrayRef<float>>* finalFeatures
        ) const {
            ui32 numFloatFeatures = Model_.GetNumFloatFeatures();
            holder->resize(numDocs * numFloatFeatures);
            FillN(holder->begin(), holder->size(), 0.0f);
            finalFeatures->resize(numDocs);

            for (size_t i = 0; i < SliceNames_.size(); ++i) {
                const auto& sliceName = SliceNames_[i];
                for (size_t docId = 0; docId < numDocs; ++docId) {
                    auto curFeatures = GetFactorsRegion(features[docId], sliceName);
                    if (curFeatures.size() > 0) {
                        ui32 startIndex = Min<ui32>(Slices_[i].StartFeatureIndexOffset, GetNumFeats());
                        ui32 endIndex = Max<ui32>(Min<ui32>(Slices_[i].EndFeatureIndexOffset, GetNumFeats()), startIndex);
                        ui32 requiredFeatures = Min<ui32>(endIndex - startIndex, curFeatures.size());
                        std::copy(
                            curFeatures.data(),
                            curFeatures.data() + requiredFeatures,
                            holder->begin() + docId * numFloatFeatures + startIndex
                        );
                    }
                }
            }
            for (size_t docId = 0; docId < numDocs; ++docId) {
               (*finalFeatures)[docId] = MakeArrayRef(holder->begin() + docId * numFloatFeatures, numFloatFeatures);
            }
        }

        virtual void DoSlicedCalcRelevs(const TFactorStorage* const* features, double* relevs, size_t numDocs) const override;

        double CalcRelev(TConstArrayRef<float> floatFeatures, TVector<TStringBuf> stringCatFeatures) const {
            NDetail::TCalcContext& context = CalcContext.GetRef();
            context.FinalFeatures.resize(1);
            context.FinalFeatures[0] = floatFeatures;
            TVector<TVector<TStringBuf>> catFeaturesV = { stringCatFeatures };
            double result;
            CalcRelevs(context.FinalFeatures, catFeaturesV, MakeArrayRef(&result, 1));
            return result;
        }

        void CalcRelevs(
                const TVector<TConstArrayRef<float>>& floatFeatures,
                const TVector<TVector<TStringBuf>>& stringCatFeatures,
                TArrayRef<double> results) const {
            Model_.Calc(floatFeatures, stringCatFeatures, results);
        }

        double CalcRelev(TConstArrayRef<float> floatFeatures, TConstArrayRef<int> hashedCatFeatures) const {
            NDetail::TCalcContext& context = CalcContext.GetRef();
            context.FinalFeatures.resize(1);
            context.FinalFeatures[0] = floatFeatures;
            TVector<TConstArrayRef<int>> hashedCatFeaturesV = { hashedCatFeatures };
            double result;
            CalcRelevs(context.FinalFeatures, hashedCatFeaturesV, MakeArrayRef(&result, 1));
            return result;
        }

        void CalcRelevs(
                const TVector<TConstArrayRef<float>>& floatFeatures,
                const TVector<TConstArrayRef<int>>& hashedCatFeatures,
                TArrayRef<double> results) const {
            Model_.Calc(floatFeatures, hashedCatFeatures, results);
        }

        void CalcRelevsFlat(const TVector<TConstArrayRef<float>>& features, TArrayRef<double> result) const override {
            Model_.CalcFlat(features, result);
        }

        void CategoricalFeatureFlatIndexes(TVector<ui32>& categoricalIndexes) const override {
            categoricalIndexes.clear();
            categoricalIndexes.reserve(Model_.ModelTrees->GetCatFeatures().size());
            for (const auto& categoricalFeature : Model_.ModelTrees->GetCatFeatures()) {
                categoricalIndexes.push_back(categoricalFeature.Position.FlatIndex);
            }
        }

        THolder<TCatboostCalcer> CopyTreeRange(size_t begin, size_t end) const {
            return MakeHolder<TCatboostCalcer>(Model_.CopyTreeRange(begin, end));
        }

        const TFullModel& GetModel() const {
            return Model_;
        }

        void Save(IOutputStream *out) const;

        void Load(IInputStream *in);

        static const TString& GetFileExtension() {
            const static TString ext(".cbm");
            return ext;
        }

        bool HasCustomSlices() const {
            if (const TString* allowCustomNamesStr = Model_.ModelInfo.FindPtr("CustomSlices")) {
                return IsTrue(*allowCustomNamesStr);
            }
            return false;
        }

        template <class TFactorStorageType>
        void CustomSlicedCalcRelevs(const TFactorStorageType features[], TVector<double>& relevs, const size_t numDocs) const {
            Y_ENSURE(!Slices_.empty());
            relevs.resize(numDocs);
            NDetail::TCalcContext& context = CalcContext.GetRef();
            GetSlicedFeatures<TFactorStorageType>(features, numDocs, &context.Holder, &context.FinalFeatures);
            DoCalcRelevs(context.FinalFeatures, relevs.data(), numDocs);
        }

        void SetSlicesFromInfo();

        void InitSlices(const TString& slicesStr, bool allowCustomNames);
    };

    using TCatboostCalcerPtr = TAtomicSharedPtr<const TCatboostCalcer>;
}
