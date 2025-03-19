#pragma once

#include "accessor.h"
#include "constraint_contents.h"
#include "reqbundle_fwd.h"

namespace NReqBundle {
    template <typename DataType>
    class TConstraintAccBase
        : public NDetail::TLightAccessor<DataType>
    {
    public:
        using NDetail::TLightAccessor<DataType>::Contents;

        TConstraintAccBase() = default;
        TConstraintAccBase(const TConstraintAcc& other)
            : NDetail::TLightAccessor<DataType>(other.Contents())
        {}
        TConstraintAccBase(const TConstConstraintAcc& other)
            : NDetail::TLightAccessor<DataType>(other.Contents())
        {}
        TConstraintAccBase(DataType& data)
            : NDetail::TLightAccessor<DataType>(data)
        {}

        EConstraintType GetType() const {
            return Contents().Type;
        }

        const TVector<size_t>& GetBlockIndices() const {
            return Contents().BlockIndices;
        }
    };
} // namespace NReqBundle
