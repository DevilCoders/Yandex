#pragma once

#include "sequence_contents.h"
#include "sequence_accessors.h"

namespace NReqBundle {
    class TSequence
        : public TAtomicRefCount<TSequence>
        , public TSequenceAcc
    {
    private:
        NDetail::TSequenceData Data;

    public:
        TSequence()
            : TSequenceAcc(Data)
        {}
        TSequence(const NDetail::TSequenceData& data)
            : TSequenceAcc(Data)
            , Data(data)
        {}
        TSequence(TConstSequenceAcc other)
            : TSequence(NDetail::BackdoorAccess(other))
        {}
        TSequence(const TSequence& other)
            : TSequence(other.Data)
        {}

        template <typename ContType>
        TSequence(const ContType& cont)
            : TSequenceAcc(Data)
        {
            for (auto& item : cont) {
                AddElem(item);
            }
        }

        TSequence& operator=(const TSequence& other) {
            Data = other.Data;
            return *this;
        }
    };
} // NReqBundle
