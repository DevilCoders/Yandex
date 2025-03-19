#pragma once

#include "reqbundle_contents.h"
#include "reqbundle_accessors.h"

namespace NReqBundle {
    class TReqBundle
        : public TAtomicRefCount<TReqBundle>
        , public TReqBundleAcc
    {
    private:
        NDetail::TReqBundleData Data;

    public:
        TReqBundle()
            : TReqBundleAcc(Data)
        {
            Data.Sequence = new TSequence();
        }
        TReqBundle(const NDetail::TReqBundleData& data)
            : TReqBundleAcc(Data)
            , Data(data)
        {}
        TReqBundle(const TConstReqBundleAcc other)
            : TReqBundle(NDetail::BackdoorAccess(other))
        {}
        TReqBundle(const TReqBundle& other)
            : TReqBundle(other.Data)
        {}

        TReqBundle(const TSequencePtr& seqPtr)
            : TReqBundleAcc(Data)
        {
            Data.Sequence = seqPtr;
        }

        TReqBundle& operator=(const TReqBundle& other) {
            Data = other.Data;
            return *this;
        }
    };
} // NReqBundle

