#pragma once

#include "constraint_accessors.h"
#include "constraint_contents.h"

namespace NReqBundle {
    class TConstraint
        : public TAtomicRefCount<TConstraint>
        , public TConstraintAcc
    {
    private:
        NDetail::TConstraintData Data;

    public:
        TConstraint()
            : TConstraintAcc(Data)
        {
        }
        TConstraint(const NDetail::TConstraintData& data)
            : TConstraintAcc(Data)
            , Data(data)
        {}
        TConstraint(TConstConstraintAcc other)
            : TConstraint(NDetail::BackdoorAccess(other))
        {}
        TConstraint(const TConstraint& other)
            : TConstraint(other.Data)
        {}

        TConstraint& operator=(const TConstraint& other) {
            Data = other.Data;
            return *this;
        }
    };

    using TConstraintPtr = TIntrusivePtr<TConstraint>;
} // namespace NReqBundle
