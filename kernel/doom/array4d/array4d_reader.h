#pragma once

#include <util/generic/noncopyable.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>

#include <library/cpp/on_disk/4d_array/array4d_poly.h>

#include <kernel/indexann_data/data.h>

#include <kernel/doom/hits/ann_data_hit.h>
#include <kernel/doom/progress/progress.h>

#include <kernel/indexann/interface/reader.h>

namespace NDoom {


class TArray4dReader: private TNonCopyable {
public:
    using THit = TAnnDataHit;

    TArray4dReader(const TString& path) {
        TArray4DPoly::TTypeInfo requiredTypes = NIndexAnn::GetAvailableTypes();

        Array_.Load(path, requiredTypes, true, false);
    }

    TArray4dReader(const TBlob& content, const TString& debugName) {
        TArray4DPoly::TTypeInfo requiredTypes = NIndexAnn::GetAvailableTypes();

        Array_.Load(content, debugName, requiredTypes, true, false);
    }


    void Restart() {
        State_ = StartedState;
        NextDocId_ = 0;
    }

    bool ReadDoc(ui32* docId) {
        while (NextDocId_ < Array_.GetCount()) {
            Elements_ = Array_.GetSubLayer(NextDocId_);
            NextDocId_++;

            if (Elements_.GetCount() == 0)
                continue;

            Break_ = 0;
            State_ = ProcessBreakState;
            *docId = NextDocId_ - 1;
            return true;
        }

        return false;
    }

    bool ReadHit(THit* hit) {
        if (!Advance())
            return false;

        size_t stream = Items_.GetKey(StreamIndex_);
        TArray4DPoly::TData data = Items_.GetData(StreamIndex_);

        Y_ENSURE_EX(data.Length <= 4, yexception() << "Annotation data larger than 4 bytes is not supported (data of size " << data.Length << " encountered at [" << (NextDocId_ - 1) << "." << Break_ << "." << Region_ << "." << stream << "])");

        *hit = TAnnDataHit(NextDocId_ - 1, Break_, Region_, stream, NIndexAnn::DataRegionToUi32(data.Start, data.Length));
        return true;
    }

    TProgress Progress() const {
        return TProgress(NextDocId_, Array_.GetCount());
    }

private:
    enum EState {
        StartedState,
        ProcessBreakState,
        ProcessRegionState,
        ProcessStreamState,
        AdvanceStreamState,
    };

    bool Advance() {
        while (true) {
            switch (State_) {
            case StartedState:
                return false;

            case ProcessBreakState:
                if (Break_ < Elements_.GetCount()) {
                    Entries_ = Elements_.GetSubLayer(Break_);
                    RegionIndex_ = 0;
                    State_ = ProcessRegionState;
                } else {
                    return false;
                }
                break;

            case ProcessRegionState:
                if (RegionIndex_ < Entries_.GetCount()) {
                    Region_ = Entries_.GetKey(RegionIndex_);
                    Items_ = Entries_.GetSubLayer(RegionIndex_);
                    StreamIndex_ = 0;
                    State_ = ProcessStreamState;
                } else {
                    Break_++;
                    State_ = ProcessBreakState;
                }
                break;

            case ProcessStreamState:
                if (StreamIndex_ < Items_.GetCount()) {
                    State_ = AdvanceStreamState;
                    return true;
                } else {
                    RegionIndex_++;
                    State_ = ProcessRegionState;
                }
                break;

            case AdvanceStreamState:
                StreamIndex_++;
                State_ = ProcessStreamState;
                break;
            }
        }
    }

private:
    TArray4DPoly Array_;

    EState State_ = StartedState;
    size_t NextDocId_ = 0;
    size_t Break_ = 0;
    size_t RegionIndex_ = 0;
    size_t Region_ = 0;
    size_t StreamIndex_ = 0;
    TArray4DPoly::TElementsLayer Elements_;
    TArray4DPoly::TEntriesLayer Entries_;
    TArray4DPoly::TItemsLayer Items_;
};


} // namespace NDoom
