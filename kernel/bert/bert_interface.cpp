#include "bert_interface.h"

#include <util/generic/yexception.h>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <utility>

namespace NBertApplier {

TBertInput::TBertInput(size_t expectedBatchSize, size_t maxInputLength, bool useSegmentIds)
    : MaxInputLength_(maxInputLength)
    , Inputs_(Reserve(expectedBatchSize))
    , InputLengths_(Reserve(expectedBatchSize))
{
    if (useSegmentIds) {
        SegmentIds_.ConstructInPlace(Reserve(expectedBatchSize));
    }
}

void TBertInput::Add(TArrayRef<int const> data,
                     const TMaybe<TArrayRef<int const>>& segmentIds) {
    constexpr int BOS = 0;
    constexpr int PaddingSegment = 0;
    Y_ENSURE(data.size() > 0, "Data should not be empty");
    Y_ENSURE(data.size() < MaxInputLength_, "Data length (" << data.size() << ") should be less than MaxInputLength (" << MaxInputLength_ << ")");
    Y_ENSURE(segmentIds.Defined() == SegmentIds_.Defined());

    size_t const lengthWithBos = data.size() + 1;

    TVector<int>& newInput = Inputs_.emplace_back(Reserve(lengthWithBos));

    newInput.push_back(BOS); // BOS
    newInput.insert(newInput.end(), data.begin(), data.end());
    InputLengths_.push_back(lengthWithBos);

    if (SegmentIds_.Defined()) {
        TVector<int>& newSegment = SegmentIds_->emplace_back(Reserve(lengthWithBos));
        newSegment.push_back(PaddingSegment);
        newSegment.insert(newSegment.end(), segmentIds->begin(), segmentIds->end());
    }
}

void TBertInput::AddForSplitBertL3Mask(TArrayRef<i32 const> splitQueryPartInputMask,
                                       TArrayRef<i32 const> splitDocPartInputMask,
                                       size_t               splitQueryLength,
                                       size_t               splitDocLength) {
    if (!SplitBertL3Mask_.Defined()) {
        SplitBertL3Mask_ = TVector<TVector<int>>{};
    }
    Y_ASSERT((splitQueryLength > 0) == (splitDocLength > 0));

    size_t maskLength = (splitQueryLength > 0)
        ? splitQueryLength + splitDocLength
        : splitQueryPartInputMask.size() + splitDocPartInputMask.size();

    Y_ENSURE(maskLength <= MaxInputLength_, "Data length should be less than MaxInputLength");

    TVector<int>& newMask = SplitBertL3Mask_->emplace_back(Reserve(maskLength));

    auto queryMaskBegin = splitQueryPartInputMask.begin();
    auto queryMaskEnd = (splitQueryLength > 0)
        ? splitQueryPartInputMask.begin() + splitQueryLength
        : splitQueryPartInputMask.end();

    auto docMaskBegin = splitDocPartInputMask.begin();
    auto docMaskEnd = (splitDocLength > 0)
        ? splitDocPartInputMask.begin() + splitDocLength
        : splitDocPartInputMask.end();

    std::transform(queryMaskBegin, queryMaskEnd,
                   std::back_inserter(newMask),
                   [](i64 input) { return static_cast<i64>(1 - input); });

    std::transform(docMaskBegin, docMaskEnd,
                   std::back_inserter(newMask),
                   [](i64 input) { return static_cast<i64>(1 - input); });

    InputLengths_.push_back(maskLength);
}

void swap(TBertInput& l, TBertInput& r) noexcept {
    l.Inputs_.swap(r.Inputs_);
    l.InputLengths_.swap(r.InputLengths_);
    l.SegmentIds_.swap(r.SegmentIds_);
    std::swap(l.MaxInputLength_, r.MaxInputLength_);

    std::swap(l.SplitBertL3EmbeddingsFP32_, r.SplitBertL3EmbeddingsFP32_);
    std::swap(l.SplitBertL3EmbeddingsFP16_, r.SplitBertL3EmbeddingsFP16_);
    std::swap(l.SplitBertL3Mask_, r.SplitBertL3Mask_);
}

}
