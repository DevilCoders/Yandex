#include "catboost_calcer.h"

#include <util/stream/input.h>
#include <util/generic/array_ref.h>

namespace NCatboostCalcer {
    void TCatboostCalcer::Save(IOutputStream* out) const {
        Model_.Save(out);
    }

    void TCatboostCalcer::Load(IInputStream* in) {
        Model_.Load(in);
        SetSlicesFromInfo();
    }

    void TCatboostCalcer::DoSlicedCalcRelevs(const TFactorStorage* const* features, double* relevs, size_t numDocs) const {
        if (Y_UNLIKELY(!features || numDocs < 1)) {
            return;
        }

        for (size_t doc = 0; doc < numDocs; ++doc) {
            if (Y_UNLIKELY(!ValidAndHasFactors(features[doc]))) {
                return;
            }
        }

        Y_ASSERT(GetSliceNames().size() == GetSlices().size());

        if (GetSlices().empty()) {
            IRelevCalcer::DoSlicedCalcRelevs(features, relevs, numDocs);
            return;
        }
        NDetail::TCalcContext& context = CalcContext.GetRef();
        GetSlicedFeatures(features, numDocs, &context.Holder, &context.FinalFeatures);
        DoCalcRelevs(context.FinalFeatures, relevs, numDocs);
    }

    void TCatboostCalcer::SetSlicesFromInfo() {
        Slices_.clear();
        SliceNames_.clear();
        if (const TString* slices = Model_.ModelInfo.FindPtr("Slices")) {
            InitSlices(*slices, HasCustomSlices());
        }
    }

    void TCatboostCalcer::InitSlices(const TString& slicesStr, bool allowCustomNames) {
        NMLPool::TFeatureSlices namedSlices;
        if (!NFactorSlices::TryToDeserializeFeatureSlices(slicesStr, allowCustomNames, namedSlices)) {
            return;
        }
        auto slicesCmp = [](const NMLPool::TFeatureSlice& a, const NMLPool::TFeatureSlice& b) {
            TSliceOffsets offsetA(a.Begin, a.End);
            TSliceOffsets offsetB(b.Begin, b.End);
            return NFactorSlices::TCompareSliceOffsets()(offsetA, offsetB);
        };
        Sort(namedSlices, slicesCmp);

        NFactorSlices::TSliceOffsets prevOffsets;

        SliceNames_.clear();
        Slices_.clear();
        for (const auto& slice : namedSlices) {
            const TSliceOffsets offsets(slice.Begin, slice.End);

            if (prevOffsets.Overlaps(offsets)) {
                // We do not support intersecting slices
                SliceNames_.clear();
                Slices_.clear();
                return;
            }
            if (!offsets.Empty()) {
                prevOffsets = offsets;
                SliceNames_.push_back(slice.Name);
                Slices_.push_back(NMatrixnet::TFeatureSlice(
                    prevOffsets.Begin,
                    0,
                    0,
                    prevOffsets.End,
                    0,
                    0));
            }
        }
    }

    NThreading::TThreadLocalValue<NDetail::TCalcContext> TCatboostCalcer::CalcContext;

}
