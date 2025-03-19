#pragma once

#include <kernel/indexann/interface/reader.h>
#include <kernel/indexann/protos/portion.pb.h>

#include <util/generic/algorithm.h>
#include <util/generic/xrange.h>

namespace NIndexAnn {
    class TOneDocMrIterator: public IDocDataIterator {
    public:
        TOneDocMrIterator(const NIndexAnn::T4DArrayRow* row)
            : Row_(row)
        {
        }

        const NIndexAnn::THit& Current() const override {
            return Current_;
        }

        bool Valid() const override {
            return Valid_;
        }

        const THit* Next() override {
            if (!Valid_) {
                return nullptr;
            }
            ++Position_;
            if (Position_ >= Row_->ElementsSize()) {
                Valid_ = false;
                return nullptr;
            }
            Current_ = ConvertHit(Row_->GetElements(Position_));
            Valid_ = Mask_.Matches(Current_);
            return (Valid_ ? &Current_ : nullptr);
        }

        void Restart(const NIndexAnn::THitMask& mask = NIndexAnn::THitMask()) override {
            Y_ASSERT(!mask.HasRegion() && !mask.HasStream());
            Y_ASSERT(mask.DocId() == 0);

            Mask_ = mask;

            auto less = [&](size_t index, void*) {
                return Row_->GetElements(index).GetElementId() < mask.Break();
            };
            auto range = xrange<size_t>(0, Row_->ElementsSize());
            Position_ = *LowerBound(range.begin(), range.end(), nullptr, less);

            if (Position_ >= Row_->ElementsSize()) {
                Valid_ = false;
            } else {
                Current_ = ConvertHit(Row_->GetElements(Position_));
                Valid_ = mask.Matches(Current_);
            }
        }

    private:
        static THit::TBreak Break(const T4DArrayElement& elem) {
            return elem.GetElementId();
        }

        static THit::TRegion Region(const T4DArrayElement& elem) {
            return elem.GetEntryKey();
        }

        static THit::TStream Stream(const T4DArrayElement& elem) {
            return elem.GetItemKey();
        }

        static THit::TValue Value(const T4DArrayElement& elem) {
            const TString& data = elem.GetData();
            return DataRegionToUi32(data.data(), data.size());
        }

        THit ConvertHit(const T4DArrayElement& elem) {
            return THit(0, Break(elem), Region(elem), Stream(elem), Value(elem));
        }

        const NIndexAnn::T4DArrayRow* Row_ = nullptr;

        bool Valid_ = false;
        size_t Position_ = Max<size_t>();
        NIndexAnn::THitMask Mask_;
        NIndexAnn::THit Current_;
    };
} // namespace NIndexAnn
