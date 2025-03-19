#include "slice_iterator.h"

#include "factor_slices.h"

namespace NFactorSlices {
    const TSlicesInfo& TSliceIteratorBase::Info = *GetSlicesInfo();

    TSliceIteratorBase::TSliceIteratorBase(EFactorSlice parent)
        : Slice(parent)
    {
        EndSlice = parent;
        while (EndSlice != EFactorSlice::COUNT &&
            Info.GetNextSibling(EndSlice) == EFactorSlice::COUNT)
        {
            EndSlice = Info.GetParent(EndSlice);
        }
        if (EndSlice != EFactorSlice::COUNT) {
            EndSlice = Info.GetNextSibling(EndSlice);
        }
    }

    void TSliceIteratorBase::Next()
    {
        if (Y_LIKELY(Valid())) {
            Slice = Info.GetNext(Slice);
            if (Slice == EndSlice) {
                Slice = EFactorSlice::COUNT;
            }
        }
    }

    TChildIteratorBase::TChildIteratorBase(EFactorSlice parent)
        : TSliceIteratorBase(parent)
    {
        if (Valid()) {
            TSliceIteratorBase::Next();
        }
    }

    TSiblingIteratorBase::TSiblingIteratorBase(EFactorSlice parent)
        : TSliceIteratorBase(parent)
    {
        if (Valid()) {
            TSliceIteratorBase::Next();
        }
    }

    void TSiblingIteratorBase::Next()
    {
        Slice = Info.GetNextSibling(Slice);
    }

    TLeafIteratorBase::TLeafIteratorBase(EFactorSlice parent)
        : TSliceIteratorBase(parent)
    {
        if (Slice != EFactorSlice::COUNT) {
            Slice = Info.GetFirstLeaf(Slice);
        }
    }

    void TLeafIteratorBase::Next() {
        if (Y_LIKELY(Valid())) {
            if (Info.GetNext(Slice) == EndSlice) {
                Slice = EFactorSlice::COUNT;
                return;
            }

            Slice = Info.GetNextLeaf(Slice);
        }
    }
}
